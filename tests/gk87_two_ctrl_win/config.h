// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "test_common.h"

// 和 keymaps/two/config.h 保持一致的地方：按下另一个键时，立刻把 tap-hold 判成「按住」，
// 不要等 TAPPING_TERM。真实 keymap 用的是 per-key 版本（只对两个分裂空格和 ctrl 层的
// Win 生效），这里用全局版本即可 —— 本测试里唯一的 tap-hold 就是 ctrl 层的 Win，
// 两条路径在 action_tapping.c 里走的是同一个分支，时序完全一样。
#define HOLD_ON_OTHER_KEY_PRESS
