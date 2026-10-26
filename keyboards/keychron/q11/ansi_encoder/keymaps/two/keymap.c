/* Copyright 2026 jomeuan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// ─────────────────────────────────────────────────────────────────────────────
// Keychron Q11 (ANSI + Encoder)「two」键位设计的固件实现
// ─────────────────────────────────────────────────────────────────────────────
//
// 设计源：KLE gist 1da15b6b07c6f42a59b51b4528fc9481
//        （docs/keychron-q11-two.json 是同一份数据的存档）
// 层设计与 skyloong GK87 分支的 keymap「two」保持一致：
//
//   层号   名字     怎么进                                  怎么出
//   ───────────────────────────────────────────────────────────────────────
//   0      INPT_L   shift 层（默认层，普通输出）            —
//   1      CTRL_L   ctrl 层：点按一次左空格                 点按一次右空格
//   2      SYB1_L   符号层：点按一次 F                      敲一个键后自动退出
//
// KLE 每个键画了三行图例，对应关系是：
//   视觉第 1 行 = shift 层（INPT_L）
//   视觉第 2 行 = 符号层（SYB1_L）
//   视觉第 3 行 = ctrl 层（CTRL_L）
//   空白 = 该层无输出（非基础层用 XXXXXXX 显式封死，避免漏出下层字符）。
//
// ── 与 GK87 相同的核心特性 ────────────────────────────────────────────────────
//
// 1) 两个分裂空格都是「点按 / 按住」双功能键：
//      左空格：按住 = Ctrl；点按 = 进 ctrl 层（在 ctrl 层里点按 = 一记 Ctrl 单击，
//              按住 = Ctrl + 临时把 ctrl 层让回 shift 层，于是「按住左空格 + J」
//              得到 Ctrl+J，而不是 ctrl 层的 ↓）。
//      右空格：shift 层上是实打实的 KC_RSFT；ctrl 层上是 SPC_SHIFT
//              （按住 = Shift，点按 = 回 shift 层）。
//    config.h 里的 HOLD_ON_OTHER_KEY_PRESS_PER_KEY 让这些修饰键**按下即生效**，
//    不用等 TAPPING_TERM。实现见文件底部的 get_hold_on_other_key_press()。
//
// 2) F 位的符号层用 OSL（一次性层），不是 LT：
//    按下即进层，敲任意键后自动退出；连点两下 F 就是在符号层里输出 `/`。
//    ⚠ 不要打开 ONESHOT_TAP_TOGGLE，否则双击 F 会变成锁定层、发出不来 `/`。
//
// 3) ctrl 层的物理 Win 键（[5,3]）也是双功能：点按 = 一记 Win 单击，
//    按住 = Win + 临时回 shift 层（「按住 Win + J」= Win+J）。
//    它的 layer_off 必须延后到 post_process_record_user，原因见下方注释。
//
// 4) Caps 位（[3,1]，KLE 写作 “Esc/ctrl+alt+”）= 点按 Esc、按住 Ctrl+Alt。
//    所以「按住它 + S」就是 Ctrl+Alt+S（截图），不需要单独的 esc 层。
//
// ── 其它约定 ─────────────────────────────────────────────────────────────────
//
// · 切层只动 layer_state（switch_layer），不碰 default_layer_state，也不写 EEPROM。
// · 厂商 q11.c 的拨码开关会把默认层切到 0/2，本设计没有第二套基础层，
//   因此用 dip_switch_update_user() 直接把它拦掉。
// · KLE 上 shift 层的 ← / → 两格是空白，但方向键在物理上是独立的四颗键，
//   这里补成 KC_LEFT / KC_RGHT（与 ↑ / ↓ 对称），否则它们在默认层不可用。
// · 旋钮按下（[0,0] / [6,8]）在 KLE 上标的是 Mute；左上那颗 [0,0] 在符号层
//   额外提供 QK_BOOT（KLE 写 “BOOT”），作为免拆机的烧录入口。
// · 底行右 Alt（[11,2]）按 KLE 已删掉，三层都是 XXXXXXX（物理上是颗空键）。
// · 符号层里带 Shift 的符号（? : + _ ( ) { } | < > " ~）不走 QMK 的 S() 键码：
//   QMK 会把 Shift 和字符键放进同一帧报告，VDI / RDP 会漏掉修饰键（? 打成 /）。
//   改成和 TextStream 一样「先单独立起 Shift，再按字符键」，实现见下方 sym_task()。
// ─────────────────────────────────────────────────────────────────────────────

#include QMK_KEYBOARD_H

#include "textstream.h"

enum layers {
    INPT_L, // ① shift 层（默认层）
    CTRL_L, // ② ctrl 层
    SYB1_L, // ③ 符号层（OSL，一次性）
};

enum custom_keycodes {
    SYM_PIN = QK_USER, // 符号层 Esc 位（KLE 的 “Pin”）：发送字符串 "030828"
    TS_PASTE,          // [1,0] 粘贴键：让键盘把缓冲里的文本开始“打”出来（打完再按 = 重打一遍）
    TS_ABORT,          // [2,0] 中断键（粘贴键正下方）：立刻停手，缓冲保留
};

// ── 组合键定义 ───────────────────────────────────────────────────────────────

// KC_NO 只是 MT 的「点按」占位：点按会被 process_record_user 改成切层动作，
// 「按住」分支由 MT 原生逻辑直接注册修饰键。
#define SPC_CTRL  MT(MOD_LCTL, KC_NO) // 左空格：按住 Ctrl，点按 → ctrl 层
#define SPC_SHIFT MT(MOD_RSFT, KC_NO) // 右空格（ctrl 层）：按住 Shift，点按 → shift 层
#define CTRL_WIN  MT(MOD_LGUI, KC_NO) // ctrl 层物理 Win：按住 Win，点按 = 一记 Win 单击

// Caps 位：点按 Esc，按住 Ctrl+Alt（KLE 的 “Esc/ctrl+alt+”）。
#define ESC_CAG MT(MOD_LCTL | MOD_LALT, KC_ESC)

// ctrl 层的 Esc 位：和 ESC_CAG 完全相同的「点按 Esc / 按住 Ctrl+Alt」，
// 但多一层行为 —— **按住期间把 ctrl 层临时让回 shift 层**。
//
// 为什么需要：ctrl 层是排他层，字母位置大多是 XXXXXXX。如果不让层，
// 「按住 Esc/Ctrl+Alt 再按 S」时 S 会按 ctrl 层解析（XXXXXXXX），
// Ctrl+Alt+S（截图）根本发不出来。让回 shift 层后 S 恢复成字母。
// ⚠ 它和 CTRL_WIN 一样，layer_off 必须延后到 post_process_record_user，
//   原因见那里；这里只是因为键码不同才需要单独一个名字。
#define CTRL_ESC MT(MOD_LCTL | MOD_LALT, KC_ESC)

// F 位：一次性符号层（按下即进层，敲一个键后自动退出）。
#define SYM_OSL OSL(SYB1_L)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // ① shift 层 = 默认层（KLE 的「上排」图例）。
    //    字母整体右移一位（Q→E、W→W、E=空格、R→F…），物理 Y / [ 无输出，
    //    O 位是退格，] 位是 P，\ 位是 PgUp，与 GK87 的 shift 层一致。
    [INPT_L] = LAYOUT_91_ansi(
        KC_MUTE,    XXXXXXX,  KC_F1,     KC_F2,     KC_F3,     KC_F4,     KC_F5,     KC_F6,       KC_F7,     KC_F8,     KC_F9,     KC_F10,    KC_F11,    KC_F12,    KC_INS,    KC_DEL,    KC_MUTE,
        TS_PASTE,   KC_CAPS,  KC_1,      KC_2,      KC_3,      KC_4,      KC_5,      KC_6,        KC_7,      KC_8,      KC_9,      KC_0,      KC_MINS,   KC_EQL,    KC_BSPC,              XXXXXXX,
        TS_ABORT,   KC_TAB,   KC_E,      KC_W,      KC_SPC,    KC_F,      KC_T,      XXXXXXX,     KC_Y,      KC_U,      KC_BSPC,   KC_O,      XXXXXXX,   KC_P,      KC_PGUP,              XXXXXXX,
        XXXXXXX,    ESC_CAG,  KC_A,      KC_S,      KC_D,      SYM_OSL,   KC_G,      XXXXXXX,     KC_H,      KC_J,      KC_K,      KC_L,      KC_I,                 KC_ENT,               XXXXXXX,
        XXXXXXX,    KC_LSFT,             KC_Z,      KC_X,      KC_C,      KC_V,      KC_B,        KC_Q,      KC_N,      KC_M,      KC_R,      KC_PGDN,              XXXXXXX,   KC_UP,
        XXXXXXX,    XXXXXXX,  XXXXXXX,   KC_LGUI,   KC_LALT,              SPC_CTRL,                         KC_RSFT,              XXXXXXX,   XXXXXXX,   KC_RCTL,   KC_LEFT,   KC_DOWN,   KC_RGHT),

    // ② ctrl 层（KLE 的「下排」图例）。
    //    ⚠ 排他层：KLE 上这一层没画图例的位置一律 XXXXXXX，不透传，
    //    否则会把 shift 层的字母漏出来（左空格/Ctrl 组合请用「按住左空格」）。
    [CTRL_L] = LAYOUT_91_ansi(
        KC_MUTE,    XXXXXXX,  KC_F1,     KC_F2,     KC_F3,     KC_F4,     KC_F5,     KC_F6,       KC_F7,     KC_F8,     KC_F9,     KC_F10,    KC_F11,    KC_F12,    KC_INS,    KC_DEL,    KC_MUTE,
        TS_PASTE,   KC_CAPS,  KC_1,      KC_2,      KC_3,      KC_4,      KC_5,      KC_6,        KC_7,      KC_8,      KC_9,      KC_0,      KC_MINS,   KC_EQL,    KC_BSPC,              KC_PGUP,
        TS_ABORT,   KC_TAB,   XXXXXXX,   XXXXXXX,   KC_SPC,    XXXXXXX,   XXXXXXX,   XXXXXXX,     XXXXXXX,   XXXXXXX,   KC_BSPC,   XXXXXXX,   XXXXXXX,   XXXXXXX,   KC_PGUP,              KC_PGDN,
        XXXXXXX,    CTRL_ESC, XXXXXXX,   XXXXXXX,   XXXXXXX,   SYM_OSL,   XXXXXXX,   XXXXXXX,     KC_LEFT,   KC_DOWN,   KC_UP,     KC_RGHT,   XXXXXXX,              KC_ENT,               KC_HOME,
        XXXXXXX,    KC_LSFT,             XXXXXXX,   XXXXXXX,   LCTL(KC_C),LCTL(KC_V),XXXXXXX,     XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,   KC_PGDN,              XXXXXXX,   KC_UP,
        XXXXXXX,    XXXXXXX,  XXXXXXX,   CTRL_WIN,  KC_LALT,              SPC_CTRL,                         SPC_SHIFT,            XXXXXXX,   XXXXXXX,   KC_RCTL,   KC_LEFT,   KC_DOWN,   KC_RGHT),

    // ③ 符号层（KLE 的「中排」图例）。
    //    一次性层：敲一个键就退出。字母/空格位置一律 XXXXXXX，避免漏出普通字符；
    //    Tab / Enter / 方向键 / 修饰键用 _______ 透传，保持可用。
    //    ↑ 位 = 变亮（RM_VALU）、↓ 位 = 变暗（RM_VALD）。
    //    ⚠ 左上旋钮按下 [0,0] = QK_BOOT（KLE 写 “BOOT”）：
    //      「点一下 F（进符号层）再按左上旋钮」= 进 Bootloader，
    //      这样不用拆键帽/拔线也能烧录固件。
    //    ⚠ [3,1]（Esc 位）= SYM_PIN（KLE 写 “Pin”）：发送字符串 "030828"。
    [SYB1_L] = LAYOUT_91_ansi(
        QK_BOOT,    _______,  _______,   _______,   _______,   _______,   _______,   _______,     _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        XXXXXXX,    XXXXXXX,  XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,     XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,   XXXXXXX,              XXXXXXX,
        XXXXXXX,    _______,  KC_TILD,   KC_GRV,    XXXXXXX,   KC_EQL,    KC_COLN,   XXXXXXX,     XXXXXXX,   KC_UNDS,   XXXXXXX,   XXXXXXX,   XXXXXXX,   KC_PLUS,   XXXXXXX,              _______,
        XXXXXXX,    SYM_PIN,  KC_BSLS,   KC_MINS,   KC_LT,     KC_SLSH,   KC_SCLN,   XXXXXXX,     KC_HOME,   KC_COMM,   KC_DOT,    KC_DQUO,   KC_END,               _______,              _______,
        XXXXXXX,    _______,             KC_LBRC,   KC_RBRC,   KC_LPRN,   KC_RPRN,   KC_GT,       KC_QUES,   KC_LCBR,   KC_RCBR,   KC_PIPE,   KC_QUOT,              XXXXXXX,   RM_VALU,
        XXXXXXX,    _______,  _______,   _______,   _______,              XXXXXXX,                          XXXXXXX,             XXXXXXX,   XXXXXXX,   _______,   _______,   RM_VALD,   _______)
};

// ── 切层 ─────────────────────────────────────────────────────────────────────
//
// 只动 layer_state，不碰 default_layer_state、不写 EEPROM。
static void switch_layer(uint8_t layer) {
    layer_clear();
    if (layer != INPT_L) {
        layer_on(layer);
    }
}

// 左空格在 ctrl 层里的两个临时状态。
static bool spc_ctrl_lctl_down = false;
static bool spc_ctrl_tmp_base  = false;

// ctrl 层 Win 键的两个临时状态 + 「延后关层」标记。
static bool ctrl_win_lgui_down   = false;
static bool ctrl_win_tmp_base    = false;
static bool ctrl_win_pending_off = false;

// ctrl 层 Esc/Ctrl+Alt 的同款状态（点按时没有额外动作，所以不需要 *_down）。
static bool ctrl_esc_tmp_base    = false;
static bool ctrl_esc_pending_off = false;

// ── 符号层：Shift 提前量 ─────────────────────────────────────────────────────
//
// 符号层里的 `?` `:` `+` 这些键码在 QMK 里是 KC_QUES = S(KC_SLASH) 这种「带修饰键的
// 16 位键码」。正常解析时 QMK 把 Shift 和字符键放进**同一帧** USB 报告里发出去：
// 本机上没问题，但 RDP / VDI 会漏采样或合并「只有修饰键变化」的那一帧，
// 结果按键事件到了、Shift 丢了 —— `?` 打成 `/`、`+` 打成 `=`。
//
// 修法与 TextStream 的带 Shift 字符完全一致（见 textstream.c 的相位说明）：
//   Shift 单独按下 -> 等 TEXTSTREAM_SHIFT_MS -> 按字符键 -> 松开字符键
//   -> 等 TEXTSTREAM_GAP_MS -> 松开 Shift
// 复用同一组宏，所以主机脚本的 SET_DELAY / -Shift 也会同时影响符号层。
//
// ⚠ 这里必须自己收尾，不能把按键交回 QMK：一次字符要跨好几个主循环周期，
//   process_record 的事件是瞬时的，没法「按住不放」。
enum {
    SYM_IDLE = 0, // 没有在发
    SYM_SHIFT,    // Shift 已单独立起，等提前量
    SYM_PRESS,    // 字符键已按下，等 press_ms
    SYM_RELEASE,  // 字符键已松开，Shift 再停留 gap_ms
};

static uint8_t  sym_phase = SYM_IDLE;
static uint8_t  sym_key   = KC_NO;
static uint32_t sym_time  = 0;

// 上一颗还没发完就要开始下一颗（或要中止）时，把已经按下的东西收干净。
static void sym_release_all(void) {
    if (sym_phase == SYM_PRESS) {
        unregister_code(sym_key);
    }
    if (sym_phase != SYM_IDLE) {
        unregister_code(KC_LSFT);
    }
    sym_phase = SYM_IDLE;
}

// 物理按下时启动一轮发送；物理松开只吞掉，不打断进行中的相位。
static void sym_send(uint16_t keycode, bool pressed) {
    if (!pressed) {
        return;
    }
    sym_release_all();
    sym_key = (uint8_t)QK_MODS_GET_BASIC_KEYCODE(keycode);
    register_code(KC_LSFT);
    sym_phase = SYM_SHIFT;
    sym_time  = timer_read32();
}

// 主循环推进（housekeeping_task_user 里调用）。
static void sym_task(void) {
    switch (sym_phase) {
        case SYM_SHIFT:
            if (timer_elapsed32(sym_time) < TEXTSTREAM_SHIFT_MS) return;
            register_code(sym_key);
            sym_phase = SYM_PRESS;
            sym_time  = timer_read32();
            return;

        case SYM_PRESS:
            if (timer_elapsed32(sym_time) < TEXTSTREAM_PRESS_MS) return;
            unregister_code(sym_key);
            sym_phase = SYM_RELEASE;
            sym_time  = timer_read32();
            return;

        case SYM_RELEASE:
            if (timer_elapsed32(sym_time) < TEXTSTREAM_GAP_MS) return;
            unregister_code(KC_LSFT);
            sym_phase = SYM_IDLE;
            return;
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        // ── 符号层的 Shift 符号：交给 sym_task 分相位发送 ────────────────
        // 按下和松开都吞掉：按下时启动状态机，松开时不做任何事
        // （字符键的按下/松开时机由 Shift 提前量决定，不跟随手指）。
        // 注意：OSL 的退层发生在 process_record 里（我们 return false 之后），
        // 所以这里不需要手动 reset_oneshot_layer()。
        //
        // 这些键码（KC_QUES = S(KC_SLASH) 等）只出现在符号层，
        // 所以不必再检查当前层；松开时 QMK 会用按下时缓存的来源层解析，
        // 拿到的仍然是同一个带 Shift 的键码。
        case KC_TILD: // ~
        case KC_COLN: // :
        case KC_UNDS: // _
        case KC_PLUS: // +
        case KC_LT:   // <
        case KC_DQUO: // "
        case KC_LPRN: // (
        case KC_RPRN: // )
        case KC_GT:   // >
        case KC_QUES: // ?
        case KC_LCBR: // {
        case KC_RCBR: // }
        case KC_PIPE: // |
            sym_send(keycode, record->event.pressed);
            return false;

        // ── 左空格：三态键 ─────────────────────────────────────────────
        // shift 层点按 = 进 ctrl 层；ctrl 层点按 = 一记 Ctrl 单击；
        // ctrl 层按住 = 保持 Ctrl + 临时让回 shift 层（这样 Ctrl+J 还是字母 J）。
        case SPC_CTRL:
            if (record->tap.count > 0) {
                if (record->event.pressed) {
                    if (layer_state_is(CTRL_L)) {
                        register_code(KC_LCTL);
                        spc_ctrl_lctl_down = true;
                    } else {
                        switch_layer(CTRL_L);
                    }
                } else if (spc_ctrl_lctl_down) {
                    unregister_code(KC_LCTL);
                    spc_ctrl_lctl_down = false;
                }
                return false;
            }
            if (record->event.pressed) {
                spc_ctrl_tmp_base = false;
                if (layer_state_is(CTRL_L)) {
                    layer_off(CTRL_L);
                    spc_ctrl_tmp_base = true;
                }
            } else if (spc_ctrl_tmp_base) {
                layer_on(CTRL_L);
                spc_ctrl_tmp_base = false;
            }
            return true;

        // ── 右空格（ctrl 层）：按住 = Shift，点按 = 回 shift 层 ──────────
        case SPC_SHIFT:
            if (record->tap.count > 0) {
                if (record->event.pressed) {
                    switch_layer(INPT_L);
                    reset_oneshot_layer();
                }
                return false;
            }
            return true;

        // ── ctrl 层的 Win 键：点按 = Win 单击，按住 = Win + 临时回 shift 层 ──
        // ⚠ 按住时的 layer_off 延后到 post_process_record_user，见下方说明。
        case CTRL_WIN:
            if (record->tap.count > 0) {
                if (record->event.pressed) {
                    register_code(KC_LGUI);
                    ctrl_win_lgui_down = true;
                } else if (ctrl_win_lgui_down) {
                    unregister_code(KC_LGUI);
                    ctrl_win_lgui_down = false;
                }
                return false;
            }
            if (record->event.pressed) {
                ctrl_win_tmp_base = false;
                if (layer_state_is(CTRL_L)) {
                    ctrl_win_pending_off = true;
                }
            } else if (ctrl_win_tmp_base) {
                layer_on(CTRL_L);
                ctrl_win_tmp_base = false;
            }
            return true;

        // ── ctrl 层的 Esc/Ctrl+Alt 位：按住 = Ctrl+Alt + 临时回 shift 层 ──
        // 点按仍是普通的 Esc（走 MT 原生），不需要拦截。
        // 按住时让回 shift 层，否则「Ctrl+Alt+S」里的 S 会落在 ctrl 层的 XXXXXXX 上。
        // ⚠ 与 CTRL_WIN 一样，layer_off 延后到 post_process_record_user。
        case CTRL_ESC:
            if (record->tap.count == 0) {
                if (record->event.pressed) {
                    ctrl_esc_tmp_base = false;
                    if (layer_state_is(CTRL_L)) {
                        ctrl_esc_pending_off = true;
                    }
                } else if (ctrl_esc_tmp_base) {
                    layer_on(CTRL_L);
                    ctrl_esc_tmp_base = false;
                }
            }
            return true; // 按住/点按的修饰键注册都交给 MT 原生逻辑

        // ── 符号层 Esc 位（KLE 里的 “Pin”） ──────────────────────────────────
        // 发送字符串 "030828"。想改成别的字符/长度就改下面这行。
        case SYM_PIN:
            if (record->event.pressed) {
                SEND_STRING("030828");
            }
            return false;

        // ── TextStream：粘贴 / 中断 ──────────────────────────────────────────
        // 这两颗键在 [1,0] / [2,0]（上下相邻），三层都是同一个 keycode，与当前层无关。
        // 只响应按下；两种事件都 return false 吞掉，避免把自己打进目标窗口。
        case TS_PASTE:
            if (record->event.pressed) {
                ts_paste();
            }
            return false;

        case TS_ABORT:
            if (record->event.pressed) {
                ts_abort();
            }
            return false;
    }
    return true;
}

// ── TextStream：主循环里推进输出状态机 ───────────────────────────────────────
//
// 放在 housekeeping 而不是 process_record：输出节奏与按键事件无关，
// 而且这里每轮主循环都会跑，天然是非阻塞的。
void housekeeping_task_user(void) {
    ts_task();
    sym_task(); // 符号层带 Shift 的键（? : + …）的发送相位
}

// ── 让分裂空格 / ctrl 层 Win 的修饰键「立即」生效 ─────────────────────────────
//
// 默认 MT 要等 TAPPING_TERM（200ms）才判定按住；对 Ctrl+C / Shift+字母 这种
// 高频组合来说太慢。按住期间只要按下了别的键就立刻按「按住」处理。
#if defined(HOLD_ON_OTHER_KEY_PRESS_PER_KEY)
bool get_hold_on_other_key_press(uint16_t keycode, keyrecord_t *record) {
    return keycode == SPC_CTRL || keycode == SPC_SHIFT || keycode == CTRL_WIN || keycode == CTRL_ESC;
}
#endif

// ── ctrl 层 Win：把「借走 ctrl 层」这一步延后 ────────────────────────────────
//
// QMK 先跑 process_record_user，再解析这颗键的 action（store_or_get_action），
// 最后才跑 post_process_record_user。如果在上一步就 layer_off(CTRL_L)，这颗键会
// 被当成 shift 层的 KC_LGUI 解析并缓存来源层，松开时读回来的就不是 CTRL_WIN，
// 「松开 → 还层」的分支永远不执行，人就被丢在 shift 层。
void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    (void)record;
    if (ctrl_win_pending_off) {
        ctrl_win_pending_off = false;
        if (layer_state_is(CTRL_L)) {
            layer_off(CTRL_L);
            ctrl_win_tmp_base = true;
        }
    }
    // ctrl 层 Esc/Ctrl+Alt 位：同一套延后关层逻辑。
    if (ctrl_esc_pending_off) {
        ctrl_esc_pending_off = false;
        if (layer_state_is(CTRL_L)) {
            layer_off(CTRL_L);
            ctrl_esc_tmp_base = true;
        }
    }
}

// ── 厂商拨码开关 / 默认层纠错 ────────────────────────────────────────────────
//
// q11.c 的 dip_switch_update_kb() 会把默认层切到 0 / 2（Mac / Win）。本设计只有
// 一套基础层，层 2 是符号层，照做会直接把键盘锁进符号层，所以这里拦掉。
#if defined(DIP_SWITCH_ENABLE)
bool dip_switch_update_user(uint8_t index, bool active) {
    (void)index;
    (void)active;
    return false;
}
#endif

// 把基础层钉死在 INPT_L：无论拨码开关还是 EEPROM 里残留了什么值都不受影响。
layer_state_t default_layer_state_set_user(layer_state_t state) {
    (void)state;
    return (layer_state_t)1 << INPT_L;
}

// EEPROM 里的默认层值不对时纠正一次（旧固件可能留下别的值）。
void keyboard_post_init_user(void) {
    if (eeconfig_read_default_layer() != ((layer_state_t)1 << INPT_L)) {
        set_single_persistent_default_layer(INPT_L);
    }
}

// ── 键位覆盖：Shift+Backspace = Delete ───────────────────────────────────────
const key_override_t delete_key_override = ko_make_basic(MOD_MASK_SHIFT, KC_BSPC, KC_DEL);
const key_override_t *key_overrides[]      = {&delete_key_override};

// ── 旋钮：三层都是音量 ───────────────────────────────────────────────────────
// ⚠ 行数必须和 keymaps[] 的层数一致，加层要记得补。
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [INPT_L] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [CTRL_L] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [SYB1_L] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
};
#endif
