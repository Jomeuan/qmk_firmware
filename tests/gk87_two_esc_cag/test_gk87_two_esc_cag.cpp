// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ─────────────────────────────────────────────────────────────────────────────
// keymaps/two「Esc/Caps 位（ESC_CAG）」在 ctrl 层的借层机制最小复现测试
//
// 设计意图：点按 = Esc，按住 = Ctrl+Alt 修饰键。
//
// ctrl 层是**排他层**：字母位是 XXXXXXX（KC_NO）。所以如果按住 Esc 时层不变，
// 「按住 Esc + 按 S」只会注册 Ctrl+Alt，S 被 KC_NO 吞掉 → 得不到 Ctrl+Alt+S
// （实测症状就是这个：只有 ctrl+alt，没有 s）。
//
// 修法和左空格一致：按住期间 layer_off(_CTRL)，让字母按 shift 层解析；
//   松开再把层还回来。
//
// 另外，ESC_CAG 只在 **ctrl 层** 启用 HOLD_ON_OTHER_KEY_PRESS：
//   · ctrl 层：字母位本来就屏蔽，「按住 Esc 再按字母」只可能是 Ctrl+Alt+字母，
//     所以按下别的键的瞬间就判成按住（不用等 200ms）；
//   · shift 层：Esc 是常用点按键，保持经典语义，快速「Esc 接字母」不会变成
//     Ctrl+Alt+字母（本测试最后一个用例钉这个差异）。
//
// 和 CTRL_WIN 不同，这里**可以**在 process_record_user 里当场 layer_off：
// _BASE[3,0] 和 _CTRL[3,0] 都是 ESC_CAG，action 按 shift 层重新解析后 keycode
// 仍然是同一个，松开时读回来照样命中「松开 → layer_on(_CTRL)」。
//
// 跑法：make test:gk87_two_esc_cag
// ─────────────────────────────────────────────────────────────────────────────

#include "keyboard_report_util.hpp"
#include "keycode.h"
#include "test_common.hpp"
#include "action_tapping.h"
#include "test_fixture.hpp"
#include "test_keymap_key.hpp"

using testing::_;
using testing::InSequence;

// ── 下面这段是 keymaps/two/keymap.c 的同款（只留了本测试要用的部分）──────────
enum layer_names {
    _BASE = 0, // shift 层
    _CTRL,     // ctrl 层
};

#define ESC_CAG MT(MOD_LCTL | MOD_LALT, KC_ESC)

static bool esc_cag_tmp_base = false;

extern "C" bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case ESC_CAG:
            if (record->tap.count == 0) {
                if (record->event.pressed) {
                    esc_cag_tmp_base = false; // 每次按下先清旧标记（防脏标记）
                    if (layer_state_is(_CTRL)) {
                        layer_off(_CTRL); // ←← 被验证的就是这一行
                        esc_cag_tmp_base = true;
                    }
                } else if (esc_cag_tmp_base) {
                    layer_on(_CTRL);
                    esc_cag_tmp_base = false;
                }
            }
            return true; // 点按 / 按住都交给 MT 原生
        default:
            return true;
    }
}

extern "C" bool get_hold_on_other_key_press(uint16_t keycode, keyrecord_t *record) {
    return keycode == ESC_CAG && layer_state_is(_CTRL);
}

class Gk87TwoEscCag : public TestFixture {};

// ── ① 核心：ctrl 层里按住 Esc + S = Ctrl+Alt+S（不是只有 Ctrl+Alt）────────────
TEST_F(Gk87TwoEscCag, hold_esc_in_ctrl_layer_then_s_emits_ctrl_alt_s) {
    TestDriver driver;
    InSequence s;

    auto esc_ctrl = KeymapKey(_CTRL, 0, 0, ESC_CAG); // ctrl 层：Caps/Esc 位
    auto esc_base = KeymapKey(_BASE, 0, 0, ESC_CAG); // shift 层同一格也是 ESC_CAG（借层后重新解析要用）
    auto s_key    = KeymapKey(_BASE, 1, 0, KC_S);    // shift 层：S 位 = 字母 S
    auto s_muted  = KeymapKey(_CTRL, 1, 0, KC_NO);   // ctrl 层：S 位 = 空（真实 keymap 是 XXXXXXX）

    set_keymap({esc_ctrl, esc_base, s_key, s_muted});

    /* 先模拟「已经点按过左空格、人在 ctrl 层」 */
    layer_on(_CTRL);
    EXPECT_TRUE(layer_state_is(_CTRL));

    /* 按住 Esc：tap-hold 还没判定，不该有任何报告 */
    EXPECT_NO_REPORT(driver);
    esc_ctrl.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 按住期间按 S：
       step 1 —— HOLD_ON_OTHER_KEY_PRESS 立刻把它判成「按住」：
                 process_record_user 先跑 → layer_off(_CTRL)，层落回 shift 层；
                 MT 原生逻辑再注册 Ctrl+Alt → 报告 #1 = Ctrl+Alt。
       step 2 —— 这时才去解析 S 属于哪一层：ctrl 层已经关了，命中的是
                 shift 层的 KC_S（不是 ctrl 层的 KC_NO）→ 报告 #2 = Ctrl+Alt+S。 */
    EXPECT_REPORT(driver, (KC_LCTL, KC_LALT));
    EXPECT_REPORT(driver, (KC_LCTL, KC_LALT, KC_S));
    s_key.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开 S：只剩 Ctrl+Alt */
    EXPECT_REPORT(driver, (KC_LCTL, KC_LALT));
    s_key.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开 Esc：keycode 读回来还是 ESC_CAG → 「松开」分支把 ctrl 层还回来，
       MT 原生再 unregister Ctrl+Alt → 空报告。 */
    EXPECT_EMPTY_REPORT(driver);
    esc_ctrl.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_CTRL)); // 「临时」借走的层确实还回来了
}

// ── ② ctrl 层里「点按」Esc = 一记 Esc（不借层、留在 ctrl 层）──────────────────
TEST_F(Gk87TwoEscCag, tap_esc_in_ctrl_layer_sends_one_esc) {
    TestDriver driver;
    InSequence s;

    auto esc_ctrl = KeymapKey(_CTRL, 0, 0, ESC_CAG);
    auto esc_base = KeymapKey(_BASE, 0, 0, ESC_CAG);
    set_keymap({esc_ctrl, esc_base});

    layer_on(_CTRL);

    /* 按下时什么都不发（还可能是按住） */
    EXPECT_NO_REPORT(driver);
    esc_ctrl.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    /* 松开（TAPPING_TERM 内）→ 判成点按 → MT 原生发 KC_ESC */
    EXPECT_REPORT(driver, (KC_ESC));
    EXPECT_EMPTY_REPORT(driver);
    esc_ctrl.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_TRUE(layer_state_is(_CTRL)); // 点按不会离开 ctrl 层
}

// ── ③ shift 层：经典语义（按住超过 TAPPING_TERM 才是 Ctrl+Alt）───────────────
TEST_F(Gk87TwoEscCag, hold_esc_in_base_layer_still_emits_ctrl_alt_s) {
    TestDriver driver;
    InSequence s;

    auto esc_base = KeymapKey(_BASE, 0, 0, ESC_CAG);
    auto s_key    = KeymapKey(_BASE, 1, 0, KC_S);
    set_keymap({esc_base, s_key});

    EXPECT_FALSE(layer_state_is(_CTRL));

    /* 按住超过 TAPPING_TERM → 判成按住，注册 Ctrl+Alt */
    EXPECT_NO_REPORT(driver);
    esc_base.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_REPORT(driver, (KC_LCTL, KC_LALT));
    idle_for(TAPPING_TERM + 1);
    VERIFY_AND_CLEAR(driver);

    /* 再按 S：shift 层的 S 本来就是字母 → Ctrl+Alt+S */
    EXPECT_REPORT(driver, (KC_LCTL, KC_LALT, KC_S));
    s_key.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_REPORT(driver, (KC_LCTL, KC_LALT));
    s_key.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    esc_base.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_FALSE(layer_state_is(_CTRL)); // shift 层自始至终没有被切走
}

// ── ④ HOLD_ON_OTHER_KEY_PRESS 只对 ctrl 层生效（这是本设计的关键取舍）─────────
TEST_F(Gk87TwoEscCag, hold_on_other_key_press_is_only_enabled_in_ctrl_layer) {
    keyrecord_t record = {};

    /* shift 层：快速「Esc 接字母」保持点按语义，不误判成 Ctrl+Alt+字母 */
    EXPECT_FALSE(layer_state_is(_CTRL));
    EXPECT_FALSE(get_hold_on_other_key_press(ESC_CAG, &record));

    /* ctrl 层：字母位本来就屏蔽，按下别的键就立刻算按住 */
    layer_on(_CTRL);
    EXPECT_TRUE(get_hold_on_other_key_press(ESC_CAG, &record));

    /* 其他键不受影响 */
    EXPECT_FALSE(get_hold_on_other_key_press(KC_S, &record));
}
