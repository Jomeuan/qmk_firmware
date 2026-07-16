# Quantum Mechanical Keyboard Firmware

[![Current Version](https://img.shields.io/github/tag/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/tags)
[![Discord](https://img.shields.io/discord/440868230475677696.svg)](https://discord.gg/qmk)
[![Docs Status](https://img.shields.io/badge/docs-ready-orange.svg)](https://docs.qmk.fm)
[![GitHub contributors](https://img.shields.io/github/contributors/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/pulse/monthly)
[![GitHub forks](https://img.shields.io/github/forks/qmk/qmk_firmware.svg?style=social&label=Fork)](https://github.com/qmk/qmk_firmware/)

This is a keyboard firmware based on the [tmk\_keyboard firmware](https://github.com/tmk/tmk_keyboard) with some useful features for Atmel AVR and ARM controllers, and more specifically, the [OLKB product line](https://olkb.com), the [ErgoDox EZ](https://ergodox-ez.com) keyboard, and the Clueboard product line.

---

## 🔑 Keychron Q11 — My Custom Keymap

> **Keyboard**: Keychron Q11 (ANSI + Encoder) &nbsp;|&nbsp; **Keymap Name**: `two`
>
> 📂 `keyboards/keychron/q11/ansi_encoder/keymaps/two/`

### 🚀 Getting Started（重新上手看这里）

#### 环境准备（一次性）

```powershell
# Windows: 安装 QMK MSYS 后运行
qmk setup

# macOS / Linux
python3 -m pip install qmk
qmk setup
```

#### 克隆 & 编译

```powershell
# 1. 克隆本仓库（含子模块）
git clone --recurse-submodules https://github.com/<你的用户名>/qmk_firmware.git
cd qmk_firmware

# 2. 编译固件
qmk compile -kb keychron/q11/ansi_encoder -km two
```

#### 刷入键盘

```powershell
# 方式一：直接刷入（先将键盘拨到 Reset / 按背后的 RESET 键）
qmk flash -kb keychron/q11/ansi_encoder -km two

# 方式二：手动刷入 — 编译后 .bin 文件在项目根目录，用 QMK Toolbox 刷入
```

### ⌨️ 层（Layer）速查

| Layer | 名称 | 用途 |
|:------|:-----|:-----|
| `CTRL_L` | Control | 多媒体 / 快捷键 / 鼠标 / Ctrl+Z X C V |
| `INPT_L` | Input | Colemak 文本输入 |
| `GAME_L` | Game | QWERTY 游戏模式 |
| `CESC_L` | Control+Esc | 调试功能（Bootloader、RGB、截图等） |
| `SYB1_L` | Symbol | 符号快捷键（~ ` ( ) [ ] { } &lt; &gt; \| : " ? \_ +） |

### 🔧 启用的功能

| 功能 | 用途 |
|:-----|:-----|
| `ENCODER_MAP_ENABLE` | 左右旋钮 → 音量调节 |
| `TAP_DANCE_ENABLE` | 单击/双击触发不同动作 |
| `MOUSEKEY_ENABLE` | 键盘控制鼠标滚轮 |
| `KEY_OVERRIDE_ENABLE` | `Shift + Backspace` → `Delete` |

### 🔄 同步上游更新

```powershell
git fetch upstream
git merge upstream/master
# 解决冲突后
git push origin master
```

---

