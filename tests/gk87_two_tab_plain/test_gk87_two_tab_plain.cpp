// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ─────────────────────────────────────────────────────────────────────────────
// keymaps/two「Tab 就是普通 Tab」的回归测试
//
// Tab 曾经是「点按 = Tab / 按住 = alt+shift」的 tap-hold（`MT(MOD_LALT | MOD_LSFT, KC_TAB)`），
// KLE 上那一格写的是 “Tab/alt+shift+”。**现在已取消**（KLE 改回了普通的 “Tab”）：
//
//   _BASE[15]（物理 Tab）= KC_TAB，_CTRL[15] 直接透传。
//
// 这个测试钉两件事：
//   ① 按 Tab 立刻出 KC_TAB、松开立刻抬起 —— 不再有 tap-hold 的等待/抢跑；
//   ② 想 Shift+Tab 就**真按住 Shift** 再按 Tab（这是 Shift+Tab 本来的按法）。
//
// 如果有人把 MT(MOD_LALT | MOD_LSFT, KC_TAB) 又改回来，① 就会失败：
// 那时按 Tab 的瞬间不会发 KC_TAB（要等 TAPPING_TERM 才判定）。
//
// 跑法：make test:gk87_two_tab_plain
// ─────────────────────────────────────────────────────────────────────────────

#include "keyboard_report_util.hpp"
#include "keycode.h"
#include "test_common.hpp"
#include "test_fixture.hpp"
#include "test_keymap_key.hpp"

using testing::_;
using testing::InSequence;

class Gk87TwoTabPlain : public TestFixture {};

// ── ① Tab 立刻生效：按下就出 KC_TAB，松开就抬起 ─────────────────────────────
//    （tap-hold 的话按下那一瞬间是不会出键的，所以这条能钉住「没有 MT」）
TEST_F(Gk87TwoTabPlain, tab_is_a_plain_key_with_no_delay) {
    TestDriver driver;
    InSequence s;

    auto tab = KeymapKey(0, 0, 0, KC_TAB);
    set_keymap({tab});

    EXPECT_REPORT(driver, (KC_TAB));
    tab.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    tab.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

// ── ② Shift+Tab：按住真 Shift（物理左 Shift）再按 Tab ───────────────────────
TEST_F(Gk87TwoTabPlain, shift_tab_uses_a_real_shift_modifier) {
    TestDriver driver;
    InSequence s;

    auto lshift = KeymapKey(0, 1, 0, KC_LSFT);
    auto tab    = KeymapKey(0, 0, 0, KC_TAB);
    set_keymap({lshift, tab});

    EXPECT_REPORT(driver, (KC_LSFT));
    lshift.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_REPORT(driver, (KC_LSFT, KC_TAB));
    tab.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_REPORT(driver, (KC_LSFT)); // 松开 Tab → 只剩 Shift
    tab.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver); // 松开 Shift
    lshift.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

// ── ③ 反例防护：Tab 按住期间按别的键，**不会**冒出 alt+shift ────────────────
TEST_F(Gk87TwoTabPlain, holding_tab_does_not_produce_alt_shift) {
    TestDriver driver;
    InSequence s;

    auto tab = KeymapKey(0, 0, 0, KC_TAB);
    auto a   = KeymapKey(0, 2, 0, KC_A);
    set_keymap({tab, a});

    EXPECT_REPORT(driver, (KC_TAB)); // 按下就出 Tab（不是等 200ms）
    tab.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 按住 Tab 再按 A：出的是「Tab 还按着 + A」，**不是** Alt+Shift+A */
    EXPECT_REPORT(driver, (KC_TAB, KC_A));
    a.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}
