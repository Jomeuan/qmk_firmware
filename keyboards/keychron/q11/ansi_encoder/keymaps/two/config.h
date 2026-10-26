// Copyright 2026 jomeuan
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// ── 一次性符号层（点按 F 进入的 SYB1_L）的超时 ────────────────────────────────
//
// QMK 默认不设超时；给个上限更安全：误点 F 也能自己恢复。
// 想要「永不超时」把下面这行删掉即可。
#define ONESHOT_TIMEOUT 5000

// ── 让分裂空格 / ctrl 层 Win 的修饰键立即生效 ────────────────────────────────
//
// 两个分裂空格是 tap-hold（按住 = Ctrl / Shift，点按 = 切层）。默认 MT 要等
// TAPPING_TERM（200ms）才知道是按住，于是「按住左空格马上按 C」会被判成点按，
// 出来的是普通 c 而不是 Ctrl+C。它们的主用途恰恰就是 Ctrl+C / Ctrl+V 和
// Shift+字母，必须瞬时，所以打开 per-key 版本。
//
// 只对 SPC_CTRL / SPC_SHIFT / CTRL_WIN 三个键生效，
// 实现在 keymap.c 的 get_hold_on_other_key_press()。
// 代价：想进 ctrl / shift 层就得先松开空格（设计里写的「点按」）。
#define HOLD_ON_OTHER_KEY_PRESS_PER_KEY

// ── 不要打开 ONESHOT_TAP_TOGGLE ──────────────────────────────────────────────
//
// 它会把「连点两下 F」变成锁定/解锁符号层，但本设计里连点两下 F 是要输出 `/`
// 的（符号层 F 位 = KC_SLSH）。开着就等于把 `/` 吃掉。

// ── TextStream ──────────────────────────────────────────────────────────────
//
// 主机用 Raw HID 把文本写进键盘的 8 KB 环形缓冲，再按 [1,0]（TS_PASTE）
// 让键盘自己“打”出来。详见 docs/textstream-design.md。
//
// 缓冲必须是 2 的幂（textstream.c 用位与代替取模）。
#define TEXTSTREAM_BUF_SIZE 8192

// 按下保持时长（ms）。必须大于 USB 1ms 轮询。
#define TEXTSTREAM_PRESS_MS 5

// 字符间隔（ms）。5+10 = 约 66 字符/秒。
// 某些终端/RDP 吞字时把它调大（例如 20），然后重烧。
#define TEXTSTREAM_GAP_MS 10

// Shift 提前量（ms）。
// 大写字母 / 需要 Shift 的符号会拆成「Shift 单独按下 -> 字符键 -> 松开」三段。
// VDI / RDP 链路实测（26 字符全 Shift 压测）：10 ms 稳定，5 ms 开始出错；默认 50（保守）。
// 更慢的链路可用脚本的 -Shift 覆盖（运行时写入 RAM，掉电复位）。
#define TEXTSTREAM_SHIFT_MS 50

// 最近多少毫秒内有物理按键 / 用户按着 Ctrl·Alt·Gui 时，自动让路暂停。
#define TEXTSTREAM_IDLE_GUARD_MS 120
