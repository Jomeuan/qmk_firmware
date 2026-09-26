// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "test_common.h"

// 和 keymaps/two/config.h 保持一致的地方：按下另一个键时，立刻把分裂空格判成「按住」，
// 不要等 TAPPING_TERM。（真实 keymap 用的是 per-key 版本，这里用全局版本即可 ——
// 这份 keymap 里唯一的 tap-hold 就是那个分裂空格，两条路径在 action_tapping.c 里
// 走的是同一个分支，时序完全一样。）
#define HOLD_ON_OTHER_KEY_PRESS
