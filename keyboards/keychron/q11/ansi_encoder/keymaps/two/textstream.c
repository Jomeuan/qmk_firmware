// Copyright 2026 jomeuan
// SPDX-License-Identifier: GPL-2.0-or-later
//
// TextStream 实现：
//   · raw_hid_receive()  —— 只把字节塞进环形缓冲，绝不在这里打字
//   · ts_task()          —— 非阻塞状态机，按 5ms/10ms 的节拍吐字符
//   · ts_paste()/ts_abort() —— 由 TS_PASTE / TS_ABORT 两颗键触发
//
// 协议：主机发 DATA/START/ABORT/CLEAR/SET_DELAY 时**不回包**，只有 QUERY 回 STATUS。
// 原因：raw_hid_send() 在 IN 端点忙时会阻塞，主机只写不读时会把固件主循环卡死。

#include "textstream.h"

#include "quantum.h"
#include "raw_hid.h"
#include "send_string.h" // ascii_to_*_lut（与 SEND_STRING 共用同一套表）

#include <string.h>

// ── 编译期检查 ───────────────────────────────────────────────────────────────

_Static_assert((TEXTSTREAM_BUF_SIZE & (TEXTSTREAM_BUF_SIZE - 1)) == 0, "TEXTSTREAM_BUF_SIZE must be power of 2");
_Static_assert(TEXTSTREAM_BUF_SIZE >= 256 && TEXTSTREAM_BUF_SIZE <= 32768, "TEXTSTREAM_BUF_SIZE out of range");

// ── 协议常量 ─────────────────────────────────────────────────────────────────

enum {
    TS_CMD_DATA = 0x01,
    TS_CMD_START = 0x02,
    TS_CMD_ABORT = 0x03,
    TS_CMD_CLEAR = 0x04,
    TS_CMD_QUERY = 0x05,
    TS_CMD_SET_DELAY = 0x06,
};

// 状态码（与 STATUS[1] 一致）
enum {
    TS_ST_IDLE = 0,
    TS_ST_ARMED = 1,
    TS_ST_TYPING = 2,
    TS_ST_PAUSED = 3,
    TS_ST_DONE = 4,
    TS_ST_ERROR = 5,
};

// 错误码（与 STATUS[6] 一致）
enum {
    TS_ERR_NONE = 0,
    TS_ERR_FULL = 1,
    TS_ERR_CHAR = 2,
    TS_ERR_PROTO = 3,
};

// 输出相位
enum {
    TS_PH_NEXT = 0, // 取下一个字符并按下
    TS_PH_SHIFT,    // 已单独按下 Shift，等 shift_ms 再按字符键
    TS_PH_PRESS,    // 已按下，等 press_ms 后松开
    TS_PH_RELEASE,  // 已松开字符键，Shift 再单独保持 shift_ms
    TS_PH_GAP,      // 已松开，等 gap_ms 后取下一个
};

// 让路时视为"用户在按快捷键"的修饰键
#define TS_BLOCK_MODS (MOD_MASK_CTRL | MOD_MASK_ALT | MOD_MASK_GUI)

// 从 PROGMEM 位表里取一位（与 send_string.c 的 PGM_LOADBIT 等价）
#define TS_LUTBIT(lut, pos) ((pgm_read_byte(&((lut)[(pos) / 8])) >> ((pos) % 8)) & 0x01)

// ── 内部状态 ─────────────────────────────────────────────────────────────────

static uint8_t  ts_buf[TEXTSTREAM_BUF_SIZE];
static uint16_t ts_head;  // 写指针
static uint16_t ts_tail;  // 读指针
static uint16_t ts_count; // 本轮未读字节数

// 完成一轮后，缓冲里的内容**不擦除**：记下这一轮的长度，
// 再按一次 TS_PASTE 就从同一位置重打一遍（用户明确要求“可以重复输出上次的文本”）。
static uint16_t ts_pass_len; // 上一轮/当前轮的总长度（0 = 没有可重放的内容）

static uint8_t ts_state   = TS_ST_IDLE;
static uint8_t ts_phase   = TS_PH_NEXT;
static uint8_t ts_error   = TS_ERR_NONE;
static bool    ts_auto_paused = false; // true = 因用户输入暂停，可自动恢复

static uint16_t ts_typed = 0; // 已消费字节数（低 16 位上报）

static uint16_t ts_key       = 0;
static bool     ts_shift     = false;
static bool     ts_key_down  = false;
static bool     ts_shift_down = false;

static uint32_t ts_phase_time = 0;
static uint32_t ts_started_at = 0;

// 运行时只开放 Shift 提前量（协议 SET_DELAY），press / gap 固定为编译期常量。
static uint16_t ts_shift_ms = TEXTSTREAM_SHIFT_MS;

// ── 环形缓冲 ─────────────────────────────────────────────────────────────────
//
// 不变式：ts_head - ts_tail == ts_count（模 TEXTSTREAM_BUF_SIZE）。
// 打字过程中 tail 前进、head 不动；一轮打完 tail == head、count == 0，
// 但数据还在缓冲里，直到有新的 DATA 写进来（或 CLEAR）。

// 已完成的那一轮还占着多少空间（用于重放保留）。
// 注意：这里**不**为“可重放的旧副本”扣空间 —— 只要有新 DATA 写进来，
// 旧副本就正式作废（ts_push 里会把 pass_len 清零），空间本来就该算作空闲。
uint16_t ts_free(void) {
    return TEXTSTREAM_BUF_SIZE - ts_count;
}

uint16_t ts_pending(void) {
    return ts_count;
}

static void ts_push(uint8_t b) {
    if (ts_count == 0) {
        ts_pass_len = 0; // 新文本进来：上一轮的副本不再可重放
    }
    ts_buf[ts_head] = b;
    ts_head         = (ts_head + 1) & (TEXTSTREAM_BUF_SIZE - 1);
    ts_count++;
}

static uint8_t ts_peek(void) {
    return ts_buf[ts_tail];
}

static void ts_pop(void) {
    ts_tail = (ts_tail + 1) & (TEXTSTREAM_BUF_SIZE - 1);
    ts_count--;
}

// ── 状态上报 ─────────────────────────────────────────────────────────────────

static void ts_send_status(void) {
    uint8_t d[TEXTSTREAM_REPORT_SIZE];
    memset(d, 0, sizeof d);

    uint16_t f = ts_free();
    d[0]       = 0x80;
    d[1]       = ts_state;
    d[2]       = f & 0xFF;
    d[3]       = f >> 8;
    d[4]       = ts_typed & 0xFF;
    d[5]       = ts_typed >> 8;
    d[6]       = ts_error;

    raw_hid_send(d, sizeof d);
}

// ── 复位 ─────────────────────────────────────────────────────────────────────

static void ts_release_current(void) {
    if (ts_key_down) {
        unregister_code(ts_key);
        ts_key_down = false;
    }
    if (ts_shift_down) {
        unregister_code(KC_LSFT);
        ts_shift_down = false;
    }
}

void ts_clear(void) {
    ts_release_current();
    ts_head        = 0;
    ts_tail        = 0;
    ts_count       = 0;
    ts_pass_len    = 0;
    ts_typed       = 0;
    ts_error       = TS_ERR_NONE;
    ts_state       = TS_ST_IDLE;
    ts_phase       = TS_PH_NEXT;
    ts_auto_paused = false;
}

// ── 用户按键 ─────────────────────────────────────────────────────────────────

void ts_paste(void) {
    if (ts_state == TS_ST_ARMED || ts_state == TS_ST_PAUSED) {
        ts_state      = TS_ST_TYPING;
        ts_phase      = TS_PH_NEXT;
        ts_started_at = timer_read32();
        return;
    }

    // DONE 状态：缓冲里还留着上一轮的完整副本，再按一次就从头重打。
    // 这么做不依赖于主机再跑一遍脚本，也不会改写主机送进来的内容。
    if (ts_state == TS_ST_DONE && ts_count == 0 && ts_pass_len) {
        ts_tail        = (ts_head - ts_pass_len) & (TEXTSTREAM_BUF_SIZE - 1);
        ts_count       = ts_pass_len;
        ts_typed       = 0;
        ts_error       = TS_ERR_NONE;
        ts_state       = TS_ST_TYPING;
        ts_phase       = TS_PH_NEXT;
        ts_auto_paused = false;
        ts_started_at  = timer_read32();
    }
}

void ts_abort(void) {
    if (ts_state == TS_ST_TYPING) {
        ts_release_current();
        ts_state       = TS_ST_PAUSED;
        ts_auto_paused = false; // 手动中断：必须再按 TS_PASTE 才继续
    } else if (ts_state == TS_ST_ARMED) {
        ts_state = TS_ST_IDLE; // 撤销待命
    }
}

// ── 输出状态机 ───────────────────────────────────────────────────────────────

// 从缓冲里取下一个可输出的字符，成功返回 true。
// 跳过：\r、0x00、非 ASCII、AltGr 字符（US 布局没有）。
static bool ts_prepare_next(void) {
    while (ts_count > 0) {
        uint8_t c = ts_peek();
        ts_pop();
        ts_typed++;

        if (c == '\r' || c == 0x00) {
            continue; // CRLF 里的 \r 直接丢弃，避免敲两下回车
        }

        if (c == '\n') {
            ts_key   = KC_ENT;
            ts_shift = false;
            return true;
        }
        if (c == '\t') {
            ts_key   = KC_TAB;
            ts_shift = false;
            return true;
        }
        if (c >= 0x80) {
            ts_error = TS_ERR_CHAR;
            continue;
        }
        if (TS_LUTBIT(ascii_to_altgr_lut, c)) {
            ts_error = TS_ERR_CHAR; // US 布局用不到 AltGr
            continue;
        }

        uint8_t k = pgm_read_byte(&ascii_to_keycode_lut[c]);
        if (k == 0) {
            continue;
        }

        ts_key   = k;
        ts_shift = TS_LUTBIT(ascii_to_shift_lut, c);
        return true;
    }
    return false;
}

void ts_task(void) {
    // 1) 状态闸门
    if (ts_state == TS_ST_PAUSED) {
        if (!ts_auto_paused) {
            return; // 手动中断，等 TS_PASTE
        }
        if (last_input_activity_elapsed() < TEXTSTREAM_IDLE_GUARD_MS) return;
        if (get_mods() & TS_BLOCK_MODS) return;
        ts_state = TS_ST_TYPING; // 让路结束，自动续打
    } else if (ts_state != TS_ST_TYPING) {
        return;
    }

    // 2) 只在字符边界检查"用户是否在打字"，避免把半个字符卡在中间
    if (ts_phase == TS_PH_NEXT && timer_elapsed32(ts_started_at) > TEXTSTREAM_IDLE_GUARD_MS) {
        if (last_input_activity_elapsed() < TEXTSTREAM_IDLE_GUARD_MS || (get_mods() & TS_BLOCK_MODS)) {
            ts_state       = TS_ST_PAUSED;
            ts_auto_paused = true;
            return;
        }
    }

    // 3) 相位推进（NEXT -> [SHIFT] -> PRESS -> [RELEASE] -> GAP -> NEXT）
    //
    // 需要 Shift 的字符多走两段：先把 Shift 单独按下并保持 shift_ms，再按字符键；
    // 字符键松开后只需 gap_ms 就松开 Shift。
    //
    // 单独提前是必需的：本机上同帧就够，但 RDP / VDI 会把"只有 Shift 的那一帧"
    // 合并或漏采样（大写变小写、'!' 变 '1'）。实测 20 ms 已冗余。
    if (ts_phase == TS_PH_GAP) {
        if (timer_elapsed32(ts_phase_time) < TEXTSTREAM_GAP_MS) return;
        ts_phase = TS_PH_NEXT;
    }

    if (ts_phase == TS_PH_NEXT) {
        if (!ts_prepare_next()) {
            if (ts_count == 0) {
                // 一轮结束：记下长度，数据留在缓冲里等下一次 TS_PASTE 重放。
                ts_pass_len = ts_typed;
                ts_state    = TS_ST_DONE;
            }
            return;
        }
        if (ts_shift) {
            register_code(KC_LSFT);
            ts_shift_down = true;
            ts_phase_time = timer_read32();
            ts_phase      = TS_PH_SHIFT;
            return;
        }
        register_code(ts_key);
        ts_key_down   = true;
        ts_phase_time = timer_read32();
        ts_phase      = TS_PH_PRESS;
        return; // 下一轮再收手，保证按住 press_ms
    }

    // TS_PH_SHIFT：Shift 已单独立起，等提前量过去再按字符键
    if (ts_phase == TS_PH_SHIFT) {
        if (timer_elapsed32(ts_phase_time) < ts_shift_ms) return;
        register_code(ts_key);
        ts_key_down   = true;
        ts_phase_time = timer_read32();
        ts_phase      = TS_PH_PRESS;
        return;
    }

    // TS_PH_PRESS：到点松开字符键；带 Shift 的还要再等 gap_ms 才松 Shift
    if (ts_phase == TS_PH_PRESS) {
        if (timer_elapsed32(ts_phase_time) < TEXTSTREAM_PRESS_MS) return;
        if (ts_key_down) {
            unregister_code(ts_key);
            ts_key_down = false;
        }
        ts_phase_time = timer_read32();
        ts_phase      = ts_shift_down ? TS_PH_RELEASE : TS_PH_GAP;
        return;
    }

    // TS_PH_RELEASE：字符已经出去，Shift 再单独停留一小段后松开。
    // 这里刻意不用 shift_ms：提前量只对"字符键之前"有意义。
    if (timer_elapsed32(ts_phase_time) < TEXTSTREAM_GAP_MS) return;
    if (ts_shift_down) {
        unregister_code(KC_LSFT);
        ts_shift_down = false;
    }
    ts_phase_time = timer_read32();
    ts_phase      = TS_PH_GAP;
}

// ── Raw HID 收包 ─────────────────────────────────────────────────────────────

void raw_hid_receive(uint8_t *data, uint8_t length) {
    (void)length;

    if (!is_keyboard_master()) return; // 从半区没有 USB

    switch (data[0]) {
        case TS_CMD_DATA: {
            uint8_t n = data[1];
            if (n > TEXTSTREAM_CHUNK_MAX) {
                ts_error = TS_ERR_PROTO;
                n        = TEXTSTREAM_CHUNK_MAX;
            }
            for (uint8_t i = 0; i < n; i++) {
                if (ts_count >= TEXTSTREAM_BUF_SIZE) {
                    ts_error = TS_ERR_FULL; // 截断：不再写入
                    break;
                }
                ts_push(data[2 + i]);
            }
            break;
        }

        case TS_CMD_START:
            if (ts_count == 0) {
                ts_state = TS_ST_DONE;
            } else if (ts_state != TS_ST_TYPING) {
                ts_typed = 0; // 新一轮：typed 从 0 开始计数
                ts_state = TS_ST_ARMED;
                ts_phase = TS_PH_NEXT;
            }
            break;

        case TS_CMD_ABORT:
            ts_abort();
            break;

        case TS_CMD_CLEAR:
            ts_clear();
            break;

        case TS_CMD_QUERY:
            ts_send_status();
            break;

        case TS_CMD_SET_DELAY:
            // [1..2] = shift 提前量（16 位小端）；0 = 保持当前值。
            // press / gap 不开放运行时调整：它们的最佳值已由编译期宏定死。
            if (data[1] || data[2]) ts_shift_ms = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
            break;

        default:
            ts_error = TS_ERR_PROTO;
            break;
    }
}
