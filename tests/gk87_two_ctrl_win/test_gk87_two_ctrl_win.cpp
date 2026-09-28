// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ─────────────────────────────────────────────────────────────────────────────
// keymaps/two「ctrl 层的 Win 键」机制的最小复现测试
//
// 这一格和「ctrl 层的左空格」用的是同一套机制（见 tests/gk87_two_left_space/）：
//
//   按住 Win 再接 J —— 出来的必须是 Win+J（字母），而不是 ctrl 层 J 位上的 ↓（Win+↓）。
//
// 手段仍然是「layer_off(_CTRL)」：按住期间把 ctrl 层临时借走，让层查表落回 shift 层。
// 但这一步**不能**做在 process_record_user 里，必须放到 post_process_record_user
// （并且本测试就是钉这件事的）：
//
//   QMK 处理一颗键的顺序（action.c 的 process_record）是
//     process_record_quantum()（→ process_record_user）
//     → process_record_handler()（→ store_or_get_action，**这里才解析 action**）
//     → post_process_record_quantum()（→ post_process_record_user）
//
//   如果在第一步就 layer_off，这颗键的 action 会按 **shift 层的 KC_LGUI** 解析，
//   并把「它来自 shift 层」写进 source_layers_cache；于是**松开时** keycode 也按
//   shift 层读回来 = 普通 KC_LGUI，而不是 CTRL_WIN。
//   后果：keymap 里 CTRL_WIN 的「松开 → 把 ctrl 层还回来」分支永远不会执行，
//   层被永久借走（本测试第一个用例一开始就是这么挂的，看最后的层断言）。
//
//   另外还要看：(b) 按住那一下必须把 Win 修饰键注册上（MT 原生）；
//   (c) 之后按的 J 必须走 shift 层的层查表（HOLD_ON_OTHER_KEY_PRESS 分支的时序）。
//
// 跑法：make test:gk87_two_ctrl_win
// ─────────────────────────────────────────────────────────────────────────────

#include "keyboard_report_util.hpp"
#include "keycode.h"
#include "test_common.hpp"
#include "test_fixture.hpp"
#include "test_keymap_key.hpp"

using testing::_;
using testing::InSequence;

// ── 下面这段是 keymaps/two/keymap.c 的同款（只留了本测试要用的部分）──────────
enum layer_names {
    _BASE = 0, // shift 层
    _CTRL,     // ctrl 层
};

#define CTRL_WIN MT(MOD_LGUI, KC_NO) // ctrl 层 Win：按住 = Win，点按 = 一记 Win 单击

static bool ctrl_win_lgui_down   = false;
static bool ctrl_win_tmp_base    = false;
static bool ctrl_win_pending_off = false;

extern "C" bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case CTRL_WIN:
            if (record->tap.count > 0) {
                // 点按 = 一记 Win 单击
                if (record->event.pressed) {
                    register_code(KC_LGUI);
                    ctrl_win_lgui_down = true;
                } else if (ctrl_win_lgui_down) {
                    unregister_code(KC_LGUI);
                    ctrl_win_lgui_down = false;
                }
                return false;
            }
            // 按住
            if (record->event.pressed) {
                ctrl_win_tmp_base = false; // 每次按下先清旧标记（防漏掉的松开留下脏标记）
                if (layer_state_is(_CTRL)) {
                    ctrl_win_pending_off = true; // ←← 被验证的就是这一行：关层延后一步
                }
            } else if (ctrl_win_tmp_base) {
                layer_on(_CTRL);
                ctrl_win_tmp_base = false;
            }
            return true;
        default:
            return true;
    }
}

// action 解析完之后才真关层（详见文件开头的说明）
extern "C" void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    (void)record;
    if (ctrl_win_pending_off) {
        ctrl_win_pending_off = false;
        if (layer_state_is(_CTRL)) {
            layer_off(_CTRL);
            ctrl_win_tmp_base = true;
        }
    }
}

class Gk87TwoCtrlWin : public TestFixture {};

// ── ① 核心：ctrl 层里按住 Win + J = Win+J（不是 Win+↓）───────────────────────
TEST_F(Gk87TwoCtrlWin, hold_win_in_ctrl_layer_then_j_emits_gui_j) {
    TestDriver driver;
    InSequence s;

    auto win_ctrl = KeymapKey(_CTRL, 1, 0, CTRL_WIN);  // ctrl 层：Win 位
    auto win_base = KeymapKey(_BASE, 1, 0, KC_LGUI);   // shift 层：同一格 = 普通 Win 键
    auto j_letter = KeymapKey(_BASE, 2, 0, KC_J);      // shift 层：J 位 = 字母 J
    auto j_arrow  = KeymapKey(_CTRL, 2, 0, KC_DOWN);   // ctrl 层：J 位 = ↓

    set_keymap({win_ctrl, win_base, j_letter, j_arrow});

    /* 先模拟「已经点按过左空格、人在 ctrl 层」 */
    layer_on(_CTRL);
    EXPECT_TRUE(layer_state_is(_CTRL));

    /* 按住 Win：tap-hold 还没判定，不该有任何报告 */
    EXPECT_NO_REPORT(driver);
    win_ctrl.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 按住期间按 J：
       step 1 —— HOLD_ON_OTHER_KEY_PRESS 立刻把它判成「按住」：
                 process_record_user 设上「待关层」标记，action 仍按 ctrl 层的 CTRL_WIN
                 解析 → MT 原生 register_mods(Win) → 报告 #1 = 只有 Win/GUI；
                 post_process_record_user 紧接着 layer_off(_CTRL)，层这才让给 shift 层。
       step 2 —— 再去解析 J 属于哪一层：ctrl 层已经关了，所以命中的是
                 shift 层的 KC_J（不是 ctrl 层的 KC_DOWN）→ 报告 #2 = Win+J。 */
    EXPECT_REPORT(driver, (KC_LGUI));
    EXPECT_REPORT(driver, (KC_LGUI, KC_J));
    j_letter.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开 J：只剩 Win */
    EXPECT_REPORT(driver, (KC_LGUI));
    j_letter.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开 Win：keycode 读回来还是 ctrl 层的 CTRL_WIN → 「松开」分支把 ctrl 层还回来，
       MT 原生再 unregister_mods(Win) → 空报告。 */
    EXPECT_EMPTY_REPORT(driver);
    win_ctrl.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_CTRL)); // 「临时」借走的层确实还回来了
}

// ── ② ctrl 层里「点按」Win = 一记 Win 单击（按下 register / 松开 unregister）──
TEST_F(Gk87TwoCtrlWin, tap_win_in_ctrl_layer_sends_one_gui_tap) {
    TestDriver driver;
    InSequence s;

    auto win_ctrl = KeymapKey(_CTRL, 1, 0, CTRL_WIN);
    auto win_base = KeymapKey(_BASE, 1, 0, KC_LGUI);
    set_keymap({win_ctrl, win_base});

    layer_on(_CTRL);

    /* 按下时什么都不发（还可能是按住） */
    EXPECT_NO_REPORT(driver);
    win_ctrl.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开（TAPPING_TERM 内）→ 判成点按 → 先按下 Win，再松开 Win */
    EXPECT_REPORT(driver, (KC_LGUI));
    EXPECT_EMPTY_REPORT(driver);
    win_ctrl.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_CTRL)); // 点按不会离开 ctrl 层
}

// ── ③ 回归：shift 层那一格是实打实的 KC_LGUI（普通 Win 键），不受本机制影响 ──
TEST_F(Gk87TwoCtrlWin, win_in_base_layer_is_plain_gui_key) {
    TestDriver driver;
    InSequence s;

    auto win_base = KeymapKey(_BASE, 1, 0, KC_LGUI);
    set_keymap({win_base});

    /* 按下立刻出 Win（没有任何 tap-hold 等待） */
    EXPECT_REPORT(driver, (KC_LGUI));
    win_base.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开即抬起，层也没有被动过 */
    EXPECT_EMPTY_REPORT(driver);
    win_base.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_FALSE(layer_state_is(_CTRL));
}
