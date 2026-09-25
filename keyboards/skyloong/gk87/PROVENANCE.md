# 来源与许可说明（Provenance）

本目录（`keyboards/skyloong/gk87/`）中的键盘定义**不是**本项目原创，
而是从厂商公开仓库取得的。此文件说明其来源、许可与本仓库所做的修改。

保留此文件的目的：让此后任何人（包括未来的你）都能判断
「哪些是厂商代码、哪些是本项目改的」，并满足 GPL-2.0 的署名要求。

---

## 1. 来源

| 项 | 值 |
| --- | --- |
| 上游仓库 | <https://github.com/NaturalZh/qmk_firmware07072023> |
| ref（tag） | `gk87_bl_pro_ansi_v1.0.0` |
| release 名 | `GK87_BL_Pro_ansi_Release_V1.0.0` |
| 发布日期 | 2024-08-08 |
| 目录 | `keyboards/skyloong/gk87/` |
| 本机下载方式 | `C:\Users\jomeu\mycode\qmk-setup\fetch-gk87.ps1`（逐个文件取 raw，不 clone 整个约 280 MB + 的仓库） |

> ⚠ 该仓库采用「按键盘分 tag」的组织方式：**`master` 上并没有 `gk87` 目录**
> （`keyboards/skyloong/` 只有 `dt40` / `gk61` / `qk21`，与上游 QMK 一致）。
> 定义只存在于 tag 中，所以 `master` 里搜不到 —— 别据此认为「厂商没公开」。

同仓库中 GK87 的其它变体（本项目**未**采用）：

| tag | 差异 |
| --- | --- |
| `gk87_q1_ansi_v1.0.0` | RGB matrix 版（本项目用的是 PWM 单色背光版） |
| `gk87_bl_v1.0.0` | 同时含 ANSI + ISO |

## 2. 许可

厂商代码来自 QMK Firmware，因此继承其许可：

- **GPL-2.0-or-later**（上游 QMK 的许可，见 `qmk_firmware/LICENSE`）
- 厂商未在文件内声明与上游不同的许可
- 因此：**可以再分发、可以修改**，但需保留许可声明与出处（即本文件）

构建产物（`*.bin`）同样受 GPL-2.0 约束。

## 3. 本项目所做的修改

目前**只有一处**，且有明确依据：

| 文件 | 修改 |
| --- | --- |
| `bl_pro/ansi/keymaps/default/keymap.c` | `RGB_HUD, RGB_HUI` → `BL_DOWN, BL_UP` |

**原因**：

1. 新版 QMK（本机 0.34.4）已删除 `RGB_*` 系列键码，改名为
   `RM_HUED` / `RM_HUEU`（见 `quantum/keycodes.h`），原写法**编译不过**：
   ```
   keymaps/default/keymap.c:53:46: error: 'RGB_HUI' undeclared here (not in a function)
   ```
2. GK87 是**单色 PWM 背光、没有 RGB**，调色相本来就没有意义。
3. 这不是自由发挥 —— 厂商自己的 `keymaps/via/keymap.c` 里**本来就用**
   `ENCODER_CCW_CW(BL_DOWN, BL_UP)`，只是 `default` 那份忘了同步。

> ⚠ **注意**：重新运行 `fetch-gk87.ps1` 会用厂商原版覆盖它，
> 这个修改会丢失。所以**建议把这个目录纳入 git 管理**（见
> `docs/project-workflow.md`），而不是每次重新下载。

## 4. 硬件参数（摘自厂商 `bl_pro/info.json`，供参考）

| 项 | 值 |
| --- | --- |
| MCU | STM32F103（board `STM32_F103_STM32DUINO`） |
| bootloader | `stm32duino` |
| 矩阵 | 6 rows × 16 cols（cols 全 `null`，由自定义 `matrix.c` 扫描） |
| row 引脚 | A0 A1 A2 A3 A4 B0 |
| 矩阵 74HC595 | `HC595_ST_PIN=A6` / `SH_PIN=A5` / `DS_PIN=A7`（`matrix.c`） |
| LED 数码屏 74HC595 | `HC595_ST_PIN=B5` / `SH_PIN=B4` / `DS_PIN=B3`（`bl_pro.c`） |
| 背光 | PWM，pin **B8**，`on_state: 1`，20 级，支持呼吸；**无 RGB** |
| encoder | `pin_a=B6` / `pin_b=B7` |
| DIP 开关 | A15 |
| 指示灯 | `scroll_lock=C14`、`caps_lock=C15`，`on_state: 0` |
| USB | VID `0x1EA7`，`max_power: 380` |

> 两个 74HC595 是**独立的两片**，别混淆：`matrix.c` 的 A5/A6/A7 管矩阵扫描，
> `bl_pro.c` 的 B3/B4/B5 管 LED 数码屏。

## 5. 厂商维护状态（2026-09-25 核实）

- GK87 最后一次 release：**2024-08-08**（v1.0.0），此后无更新
- 仓库 `pushed_at` 显示 2025-06，但那些提交是**同步上游 QMK**
  （作者为 `tzarc` 等 QMK 核心维护者），与 GK87 无关
- 结论：**不要指望厂商更新**。若上游 QMK 再有破坏性改动，
  需要本项目自己适配
