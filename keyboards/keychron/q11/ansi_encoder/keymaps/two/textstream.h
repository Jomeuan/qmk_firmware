// Copyright 2026 jomeuan
// SPDX-License-Identifier: GPL-2.0-or-later
//
// TextStream —— 通过 Raw HID 把主机文本送进键盘 RAM 缓冲，再由键盘自己"打"出来。
// 设计文档：docs/textstream-design.md

#pragma once

#include <stdint.h>
#include <stdbool.h>

// ── 默认参数（可在 config.h 里覆盖） ──────────────────────────────────────────

// 环形缓冲大小。必须是 2 的幂，且不超过 32768。
#ifndef TEXTSTREAM_BUF_SIZE
#    define TEXTSTREAM_BUF_SIZE 8192
#endif

// 按下保持时长（ms）。必须 > USB 1ms 轮询，留余量。
#ifndef TEXTSTREAM_PRESS_MS
#    define TEXTSTREAM_PRESS_MS 5
#endif

// 字符间隔（ms）。约 66 cps。
#ifndef TEXTSTREAM_GAP_MS
#    define TEXTSTREAM_GAP_MS 10
#endif

// Shift 提前量（ms）。
//
// 为什么需要单独一段提前量：本机上 Shift 与字符键同帧、只保持 press_ms 就够；
// 但在 RDP / VDI 这类链路上，"只有 Shift 的那一帧报告"会被合并或漏采样，
// 表现为大写变小写、'!' 变 '1'。所以带 Shift 的字符拆成：
//   Shift 单独按下 -> 等 shift_ms -> 按字符键 -> 松开字符键 -> 等 gap_ms -> 松开 Shift
//
// 实测（VDI，26 字符全 Shift 压测）：10 ms 仍稳定，5 ms 开始出错；
// 默认取 50 ms（保守，多不了几毫秒），可用主机脚本的 SET_DELAY / -Shift 覆盖。
#ifndef TEXTSTREAM_SHIFT_MS
#    define TEXTSTREAM_SHIFT_MS 50
#endif

// 最近多少毫秒内有物理输入就让路（自动暂停）。
#ifndef TEXTSTREAM_IDLE_GUARD_MS
#    define TEXTSTREAM_IDLE_GUARD_MS 120
#endif

// 单帧最多携带的数据字节数（32B 报告 - 2B 头部）。
#define TEXTSTREAM_CHUNK_MAX 30

// Raw HID 报告长度（RAW_EPSIZE）。
#define TEXTSTREAM_REPORT_SIZE 32

// ── 对外接口 ─────────────────────────────────────────────────────────────────

// 主循环调用（keymap.c 的 housekeeping_task_user 里）。非阻塞。
void ts_task(void);

// TS_PASTE 键：ARMED/PAUSED -> TYPING。
void ts_paste(void);

// TS_ABORT 键：TYPING -> PAUSED（缓冲保留）；ARMED -> IDLE。
void ts_abort(void);

// 清空缓冲并回到 IDLE。
void ts_clear(void);

// 当前已缓冲的字节数 / 剩余空间（调试用）。
uint16_t ts_pending(void);
uint16_t ts_free(void);
