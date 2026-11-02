// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "test_common.h"

// 和 keymaps/two/config.h 一致：用 per-key 版本，由测试里的
// get_hold_on_other_key_press() 自己决定哪些键、在哪些层「按别的键就算按住」。
// 真实 keymap 里 ESC_CAG 只在 ctrl 层返回 true（shift 层保持经典点按/按住语义），
// 这个测试就是钉这一点的。
#define HOLD_ON_OTHER_KEY_PRESS_PER_KEY
