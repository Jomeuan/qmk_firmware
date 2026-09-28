// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ─────────────────────────────────────────────────────────────────────────────
// keymaps/two「game 层切换」的最小复现测试
//
// KLE 改过一次切换逻辑（入口从底行 [5,9] 挪到 `` ` `` 位、出口改成右空格）：
//
//   进 game ：shift 层 --(点 F 进符号层)--[`` ` `` 位「→GAME」]--> game 层
//   出 game ：game 层 --(点按右空格「→SHIFT」)--> shift 层
//
// 这里钉三件事：
//   ① 符号层 `` ` ``（SYM_GAME）按下 → 层确实变成 game（且符号层被清掉）
//   ② game 层右空格（SPC_SHIFT）点按 → 直接回 shift 层（不再绕 ctrl 层）
//   ③ game 层右空格「按住」仍然是 Shift（Shift+J 要出 Shift+J）
//
// 跑法：make test:gk87_two_game_switch
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
    _CTRL,     // ctrl 层（本测试用不到，只为对齐层号）
    _SYM,      // 符号层（一次性，点按 F 进入）
    _GAME,     // game 层
};

enum custom_keycodes {
    SYM_GAME = QK_USER, // 符号层 `` ` `` 位（KLE 里的 “→GAME”）：切到 game 层
};

#define SPC_SHIFT MT(MOD_LSFT, KC_NO) // 右空格（ctrl / game 层上）：按住 Shift，点按 → shift 层

// 真实 keymap 是「只动 layer_state、不碰 default_layer_state」的独占领式切层
static void switch_layer(uint8_t layer) {
    layer_clear();
    if (layer != _BASE) {
        layer_on(layer);
    }
}

extern "C" bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        // 符号层 `` ` `` 位 = →GAME
        case SYM_GAME:
            if (record->event.pressed) {
                switch_layer(_GAME);
            }
            return false;

        // 右空格：点按 = 回 shift 层（MT 的点按占位是 KC_NO，被这里拦下来）
        case SPC_SHIFT:
            if (record->tap.count > 0) {
                if (record->event.pressed) {
                    switch_layer(_BASE);
                    reset_oneshot_layer(); // 顺手清掉可能还挂着的一次性符号层状态
                }
                return false;
            }
            return true; // 按住 → 走 MT 原生 = Shift

        default:
            return true;
    }
}

class Gk87TwoGameSwitch : public TestFixture {};

// ── ① 符号层 `` ` `` = 进 game 层 ────────────────────────────────────────────
TEST_F(Gk87TwoGameSwitch, game_entry_key_switches_to_game_layer) {
    TestDriver driver;
    InSequence s;

    auto entry = KeymapKey(_SYM, 1, 0, SYM_GAME); // 符号层：`` ` `` 位
    set_keymap({entry});

    /* 先模拟「点了一下 F、人在符号层」 */
    layer_on(_SYM);
    EXPECT_TRUE(layer_state_is(_SYM));

    /* 按 `` ` `` → 立刻切到 game 层（不发任何键码） */
    EXPECT_NO_REPORT(driver);
    entry.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_GAME));
    EXPECT_FALSE(layer_state_is(_SYM)); // 一次性符号层被 switch_layer() 的 layer_clear() 清掉了

    /* 松开也不该有输出，层保持 game */
    EXPECT_NO_REPORT(driver);
    entry.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
    EXPECT_TRUE(layer_state_is(_GAME));
}

// ── ② game 层右空格点按 = 直接回 shift 层 ───────────────────────────────────
TEST_F(Gk87TwoGameSwitch, game_right_space_tap_returns_to_shift_layer) {
    TestDriver driver;
    InSequence s;

    auto right_space = KeymapKey(_GAME, 3, 0, SPC_SHIFT);
    set_keymap({right_space});

    layer_on(_GAME);

    /* 按下时什么都不发（还可能是按住） */
    EXPECT_NO_REPORT(driver);
    right_space.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开（TAPPING_TERM 内）→ 判成点按 → 回 shift 层；MT 的点按占位被拦下，所以没有键码 */
    EXPECT_NO_REPORT(driver);
    right_space.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_FALSE(layer_state_is(_GAME)); // 出 game 层了
    EXPECT_FALSE(layer_state_is(_CTRL)); // 而且没有绕经 ctrl 层 —— 直接回 shift
}

// ── ③ game 层右空格「按住」仍然是 Shift（按住 + J = Shift+J）────────────────
TEST_F(Gk87TwoGameSwitch, game_right_space_hold_is_shift) {
    TestDriver driver;
    InSequence s;

    auto right_space = KeymapKey(_GAME, 3, 0, SPC_SHIFT);
    auto j_letter    = KeymapKey(_GAME, 2, 0, KC_J); // game 层 J 位就是字母 J
    set_keymap({right_space, j_letter});

    layer_on(_GAME);

    /* 按住右空格：tap-hold 还没判定，不该有任何报告 */
    EXPECT_NO_REPORT(driver);
    right_space.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 按住期间按 J：HOLD_ON_OTHER_KEY_PRESS 立刻判成按住 → Shift，然后 Shift+J */
    EXPECT_REPORT(driver, (KC_LSFT));
    EXPECT_REPORT(driver, (KC_LSFT, KC_J));
    j_letter.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开 J */
    EXPECT_REPORT(driver, (KC_LSFT));
    j_letter.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开右空格：Shift 抬起；层没被动过（还在 game 层） */
    EXPECT_EMPTY_REPORT(driver);
    right_space.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_GAME));
}

// ── ④ 回归：game 层 [5,9]（原来写 →CTRL 的那格）现在是**普通空格** ──────────
TEST_F(Gk87TwoGameSwitch, game_bottom_row_extra_key_is_plain_space) {
    TestDriver driver;
    InSequence s;

    auto space = KeymapKey(_GAME, 4, 0, KC_SPC);
    set_keymap({space});

    layer_on(_GAME);

    EXPECT_REPORT(driver, (KC_SPC));
    space.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    space.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_GAME)); // 它就是空格，不切层
}
