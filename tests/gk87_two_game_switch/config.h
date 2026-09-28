// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "test_common.h"

// 和 keymaps/two/config.h 保持一致：按下另一个键时立刻把 tap-hold 判成「按住」，
// 不要等 TAPPING_TERM（真实 keymap 用 per-key 版本，这里用全局版本即可 ——
// 本测试里的 tap-hold 只有 game 层右空格，两条路径在 action_tapping.c 走同一分支）。
#define HOLD_ON_OTHER_KEY_PRESS
