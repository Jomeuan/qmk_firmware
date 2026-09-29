// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "test_common.h"

// 故意**不**开 HOLD_ON_OTHER_KEY_PRESS：本测试要验证的就是「Tab 背后没有 tap-hold」，
// 所以必须用默认的判定时序（按住期间按别的键不会立刻把它推成「按住」）。
