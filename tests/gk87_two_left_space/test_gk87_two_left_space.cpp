// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ─────────────────────────────────────────────────────────────────────────────
// keymaps/two「左空格」机制的最小复现测试
//
// 这套设计里唯一**靠时序**的地方就是它，所以单独钉一下：
//
//   ctrl 层里按住左空格，再按物理 J —— 出来的必须是 Ctrl+J（字母 J），
//   而不是 ctrl 层里 J 位上的方向键（Ctrl+↓）。
//
// 实现手段是：在 process_record_user 里，左空格的「按住」分支 layer_off(_CTRL)，
// 把 ctrl 层临时借走、让层查表落回 shift 层。它能不能生效，全看 QMK 是不是
// **先把这一下「按住」送进 process_record_user、之后才去解析 J 属于哪一层**
// （也就是 action_tapping.c 里 HOLD_ON_OTHER_KEY_PRESS 那个分支的行为）。
// 这个顺序一旦反过来，测试就会失败 —— 这就是本文件存在的意义。
//
// 跑法：make test:gk87_two_left_space
// ─────────────────────────────────────────────────────────────────────────────

#include "keyboard_report_util.hpp"
#include "keycode.h"
#include "test_common.hpp"
#include "test_fixture.hpp"
#include "test_keymap_key.hpp"

using testing::_;
using testing::InSequence;

// ── 下面这几行是 keymaps/two/keymap.c 的同款（只留了本测试要用的部分）───────
enum layer_names {
    _BASE = 0, // shift 层
    _CTRL,     // ctrl 层
};

#define SPC_CTRL MT(MOD_LCTL, KC_NO) // 左空格：按住 = Ctrl，点按 = 切层

static bool spc_ctrl_lctl_down = false;
static bool spc_ctrl_tmp_base  = false;

extern "C" bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case SPC_CTRL:
            if (record->tap.count > 0) {
                // 点按
                if (record->event.pressed) {
                    if (layer_state_is(_CTRL)) {
                        // ctrl 层里它就是一颗普通 Ctrl 键：点按 = 一记 Ctrl 单击
                        register_code(KC_LCTL);
                        spc_ctrl_lctl_down = true;
                    } else {
                        // 其它层：点按 = 进 ctrl 层（真实 keymap 走 switch_layer()）
                        layer_on(_CTRL);
                    }
                } else if (spc_ctrl_lctl_down) {
                    unregister_code(KC_LCTL);
                    spc_ctrl_lctl_down = false;
                }
                return false;
            }
            // 按住
            if (record->event.pressed) {
                spc_ctrl_tmp_base = false; // 每次按下先清旧标记（防漏掉的松开留下脏标记）
                if (layer_state_is(_CTRL)) {
                    layer_off(_CTRL); // ←← 被验证的就是这一行
                    spc_ctrl_tmp_base = true;
                }
            } else if (spc_ctrl_tmp_base) {
                layer_on(_CTRL);
                spc_ctrl_tmp_base = false;
            }
            return true;
        default:
            return true;
    }
}

class Gk87TwoLeftSpace : public TestFixture {};

// ── ① 核心：ctrl 层里按住左空格 + J = Ctrl+J ─────────────────────────────────
TEST_F(Gk87TwoLeftSpace, hold_left_space_in_ctrl_layer_then_j_emits_ctrl_j) {
    TestDriver driver;
    InSequence s;

    auto left_space = KeymapKey(_CTRL, 1, 0, SPC_CTRL); // ctrl 层：左空格
    auto j_letter   = KeymapKey(_BASE, 2, 0, KC_J);     // shift 层：J 位 = 字母 J
    auto j_arrow    = KeymapKey(_CTRL, 2, 0, KC_DOWN);  // ctrl 层：J 位 = ↓

    // ⚠ 关键：shift 层那一格也必须是 SPC_CTRL（真实 keymap 的 _BASE[5,3] 正是如此）。
    // 因为 layer_off(_CTRL) 之后，QMK 会拿着**已经变了的层状态**重新解析这颗键自己的
    // action（action.c 的顺序：先跑 process_record_user，再 store_or_get_action）。
    // 如果这一格是 XXXXXXX/KC_NO，MT 的「按住」分支就没了 —— Ctrl 根本不会被注册。
    auto left_space_base = KeymapKey(_BASE, 1, 0, SPC_CTRL);

    set_keymap({left_space, left_space_base, j_letter, j_arrow});

    /* 先模拟「已经点按过左空格、人在 ctrl 层」 */
    layer_on(_CTRL);
    EXPECT_TRUE(layer_state_is(_CTRL));

    /* 按住左空格：tap-hold 还没判定，不该有任何报告 */
    EXPECT_NO_REPORT(driver);
    left_space.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 按住期间按 J：
       step 1 —— HOLD_ON_OTHER_KEY_PRESS 立刻把它判成「按住」，
                 process_record_user 先跑 → layer_off(_CTRL)，层落回 shift 层；
                 MT 原生逻辑再注册 Ctrl → 报告 #1 = 只有 Ctrl。
       step 2 —— 这时才去解析 J 属于哪一层：ctrl 层已经关了，所以命中的是
                 shift 层的 KC_J（不是 ctrl 层的 KC_DOWN）→ 报告 #2 = Ctrl+J。 */
    EXPECT_REPORT(driver, (KC_LCTL));
    EXPECT_REPORT(driver, (KC_LCTL, KC_J));
    j_letter.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开 J：只剩 Ctrl */
    EXPECT_REPORT(driver, (KC_LCTL));
    j_letter.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开左空格：Ctrl 抬起，并且 ctrl 层被原样还回来 */
    EXPECT_EMPTY_REPORT(driver);
    left_space.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_CTRL)); // 「临时」借走的层确实还回来了
}

// ── ② 对照组：只按住右空格（Shift）时，J 仍然是 ctrl 层的 ↓ = Shift+↓ ────────
//    也就是用户要的「右空格保留现在这样」。
TEST_F(Gk87TwoLeftSpace, hold_right_space_only_keeps_ctrl_layer_so_j_is_down) {
    TestDriver driver;
    InSequence s;

    auto right_space = KeymapKey(_CTRL, 3, 0, MT(MOD_LSFT, KC_NO));
    auto j_arrow     = KeymapKey(_CTRL, 2, 0, KC_DOWN);

    set_keymap({right_space, j_arrow});

    layer_on(_CTRL);

    /* 按住右空格 */
    EXPECT_NO_REPORT(driver);
    right_space.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 按 J：层没有被借走（右空格不碰层），所以 J 还是 ↓ → Shift+↓ */
    EXPECT_REPORT(driver, (KC_LSFT));
    EXPECT_REPORT(driver, (KC_LSFT, KC_DOWN));
    j_arrow.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

// ── ③ ctrl 层里「点按」左空格 = 一记 Ctrl 单击（按下 register / 松开 unregister）
TEST_F(Gk87TwoLeftSpace, tap_left_space_in_ctrl_layer_sends_one_ctrl_tap) {
    TestDriver driver;
    InSequence s;

    auto left_space      = KeymapKey(_CTRL, 1, 0, SPC_CTRL);
    auto left_space_base = KeymapKey(_BASE, 1, 0, SPC_CTRL);
    set_keymap({left_space, left_space_base});

    layer_on(_CTRL);

    /* 按下时什么都不发（还可能是按住） */
    EXPECT_NO_REPORT(driver);
    left_space.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开（TAPPING_TERM 内）→ 判成点按 → 先按下 Ctrl，再松开 Ctrl */
    EXPECT_REPORT(driver, (KC_LCTL));
    EXPECT_EMPTY_REPORT(driver);
    left_space.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_CTRL)); // 点按不会离开 ctrl 层
}

// ── ⑤ 用户的场景 2：按住左空格 + 按住右空格 + 按 J = Ctrl+Shift+J ────────────
//    按右空格这一下同时干两件事：
//      (a) 它本身就是「另一个键」→ 左空格立刻被判成按住 → layer_off + 注册 Ctrl；
//      (b) 层这时已经落回 shift 层，所以右空格命中的是 shift 层的 KC_RSFT（真 Shift），
//          而不是 ctrl 层的 SPC_SHIFT —— 两条路都能拿到 Shift，结果一样。
TEST_F(Gk87TwoLeftSpace, hold_both_spaces_then_j_emits_ctrl_shift_j) {
    TestDriver driver;
    InSequence s;

    auto left_space_ctrl  = KeymapKey(_CTRL, 1, 0, SPC_CTRL);
    auto right_space_ctrl = KeymapKey(_CTRL, 3, 0, MT(MOD_LSFT, KC_NO));
    auto j_arrow          = KeymapKey(_CTRL, 2, 0, KC_DOWN);
    auto left_space_base  = KeymapKey(_BASE, 1, 0, SPC_CTRL);
    auto right_space_base = KeymapKey(_BASE, 3, 0, KC_RSFT); // shift 层：右空格 = 真右 Shift
    auto j_letter         = KeymapKey(_BASE, 2, 0, KC_J);

    set_keymap({left_space_ctrl, right_space_ctrl, j_arrow, left_space_base, right_space_base, j_letter});

    layer_on(_CTRL);

    /* 按住左空格 */
    EXPECT_NO_REPORT(driver);
    left_space_ctrl.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 再按住右空格：先 Ctrl（左空格判成按住），再 Ctrl+Shift */
    EXPECT_REPORT(driver, (KC_LCTL));
    EXPECT_REPORT(driver, (KC_LCTL, KC_RSFT));
    right_space_ctrl.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 按 J：命中的是 shift 层的 KC_J（不是 ctrl 层的 KC_DOWN）→ Ctrl+Shift+J */
    EXPECT_REPORT(driver, (KC_LCTL, KC_RSFT, KC_J));
    j_letter.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 全部松开（顺便把测试自己的状态收干净） */
    EXPECT_REPORT(driver, (KC_LCTL, KC_RSFT)); // 松开 J
    j_letter.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_REPORT(driver, (KC_LCTL)); // 松开右空格 → Shift 抬起
    right_space_ctrl.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver); // 松开左空格 → Ctrl 抬起
    left_space_ctrl.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_CTRL)); // 「临时」借走的 ctrl 层还回来了
}

// ── ④ 回归：shift 层（不在 ctrl 层）里按住左空格 = 纯 Ctrl，不动层 ───────────
TEST_F(Gk87TwoLeftSpace, hold_left_space_in_base_layer_is_plain_ctrl) {
    TestDriver driver;
    InSequence s;

    auto left_space = KeymapKey(_BASE, 1, 0, SPC_CTRL);
    auto j_letter   = KeymapKey(_BASE, 2, 0, KC_J);

    set_keymap({left_space, j_letter});

    EXPECT_NO_REPORT(driver);
    left_space.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_REPORT(driver, (KC_LCTL));
    EXPECT_REPORT(driver, (KC_LCTL, KC_J));
    j_letter.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_FALSE(layer_state_is(_CTRL)); // 没有被误开成 ctrl 层
}
