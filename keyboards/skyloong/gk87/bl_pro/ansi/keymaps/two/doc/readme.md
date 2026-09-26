# SKYLOONG GK87 的 QMK 固件

键盘是 `SKYLOONG GK87-QMK&VIA`：
<https://skyloongtech.com/skyloong-gk87-qmkvia/?attribute_backlit=Ice-Blue&attribute_switch=KTT+Rose+Silver>


## 关于 GK87 的 QMK 支持

**GK87 不在上游 QMK 里，但厂商公开了它的定义。**

厂商把它放在一个「按键盘分 tag」的仓库中 —— **仓库的 `master` 上根本没有厂商
自己的键盘定义**（`keyboards/skyloong/` 里只有 `dt40`/`gk61`/`qk21`，`gk87` 直接
404）。所以在 `master` 里翻目录、或在上游和厂商的 fork 里搜 `gk87` 都搜不到，
**必须看 tag / release 列表**。

| 项 | 值 |
| --- | --- |
| 仓库 | `NaturalZh/qmk_firmware07072023` |
| tag | `gk87_bl_pro_ansi_v1.0.0`（release 名 `GK87_BL_Pro_ansi_Release_V1.0.0`，2024-08-08） |
| 键盘路径 | `keyboards/skyloong/gk87/bl_pro/ansi` |
| 许可 | GPL-2.0-or-later（继承自上游 QMK） |


> ⚠ 厂商原版的 `keymaps/default/keymap.c` 在**新版 QMK 上编译不过**
> （用了已删除的 `RGB_HUD` / `RGB_HUI` 键码）。
> 原因与修法见 [docs/troubleshooting.md 第 1 节](docs/troubleshooting.md#1-编译-gk87-时报-rgb_hui-undeclared--rgb_hud-undeclared)。

### 硬件参数

摘自厂商 `bl_pro/info.json`：

| 项 | 值 |
| --- | --- |
| MCU | STM32F103（board `STM32_F103_STM32DUINO`） |
| bootloader | `stm32duino` |
| 矩阵 | 6 rows × 16 cols（cols 全为 `null`，由自定义 `matrix.c` 扫描） |
| row 引脚 | A0 A1 A2 A3 A4 B0 |
| 矩阵 74HC595 | `HC595_ST_PIN=A6` / `SH_PIN=A5` / `DS_PIN=A7`（见 `matrix.c`） |
| LED 数码屏 74HC595 | `HC595_ST_PIN=B5` / `SH_PIN=B4` / `DS_PIN=B3`（见 `bl_pro.c`） |
| 背光 | PWM，pin **B8**，`on_state: 1`，20 级，支持呼吸；**无 RGB** |
| encoder | `pin_a=B6` / `pin_b=B7` |
| DIP 开关 | A15 |
| 指示灯 | `scroll_lock=C14`、`caps_lock=C15`，`on_state: 0` |
| USB | VID `0x1EA7`，`max_power: 380` |

> 两个 74HC595 是**独立的两片**，别混淆：`matrix.c` 的 A5/A6/A7 管矩阵扫描，
> `bl_pro.c` 的 B3/B4/B5 管 LED 数码屏。

## 键盘特性
- 87 键 TKL（80%）ANSI 布局 + 右上角 1 个可编程旋钮
- 双空格（split spacebar）
- 旋钮焊接不可拆

`https://github.com/NaturalZh/qmk_firmware07072023/releases?page=1#release-gk87_bl_pro_ansi_v1.0.0`

Skyloong GK87 Backlight Pro keyboard V1.0.0 released.
It`s a PWM backlight keyboard with dual 3.125 space bar.
MCU is STM32F103 chip.
Supports a encoder and a LED digital screen.
It`s ANSI layout.

## 键位/层设计

详细设计键位设计见`keyboard-layout-editor/skyloong-gk87-two.json`

### 核心思想
将敲击的键位平摊到每根手指，分配给食指、大拇指的敲击任务多一些，因为肌肉发达一些。

### 键位约定
因为在keyboard layout editor上的键冒上放不下太多文字，所以用一些符号表示一些键位（包括特殊键位）
SP:space
Backspace:BS
Home:HM
End:ED

### 分层设计
分层：ctrl层、shift层、符号层、功能层、game层。（我不用mac电脑所以不用考虑兼容mac）左边的分裂空格被设置成ctrl，右边的分裂空格被设置成shift所以。
ctrl层:快按一次左空格进入ctrl层，放常用的需要带修饰键的操作（实际用下来最多的就是ctrl+c、ctrl+v）。另外还放有上下左右移动键。类似vim的正常模式
shift层:快按一次右空格进入shift层，就是普通的输出，同vim的insert模式。再按一次shift切换中英文
符号层:快按一次左手食指（对应普通键位的`f`）进入符号层，再敲击其他键输出符号，输出符号后自动回到刚刚层
功能层:当符号键作为修饰键时进入功能层，功能层的特点就是大量自定义

> 📌 **后续变更（已实现）**：功能层（按住 F 进入）**已废弃**，内容并入符号层。
> 原因：tap-hold 的 200ms 延迟 + 「右手抢跑」问题。详见下方
> 「F 用 `OSL`，不再用 tap-hold（重要）」。
game层是为打游戏准备的，打游戏就用普通键位，因为大部分游戏都是适配普通键位，不能适配我这种高度客制化的键位设计。不过我现在也基本没用上，一个原因是入口没设计好，进入和退出麻烦记不住（暂不实现）

**esc**设计
点按时作为esc键，按住时作为修饰键（ctrl+alt+）

**Tab**设计
点按时作为Tab键，按住时作为修饰键（alt+shift+）
Tab键时要保证其他修饰键能正常作用，比如先按住Shift,再点按Tab，再放开Shift，要能正确输出Shift+Tab


### 痛点
- 摸到键盘的时候不知道现在是哪一层，所以我都会先敲左/右空格，强制进入ctrl/shift层（所有层的键位这两个的作用都是进入ctrl/shift层）
- 从ctrl层到shift层我总是忘记原来的中英文，所以必须先敲一个字母，看看触发的是中文输入法还是英文输入。如果不是想要的中/英文就得删掉刚刚的输入（有时会以为当前是想要的中/英文导致输入多几个，于是要删几个），再按一次右空格切换中/英文

## 固件实现（`keymaps/two`）

设计源就是上面那份 KLE（`doc/keyboard-layout-editor/skyloong-gk87-two.json`），
`keymap.c` 是它的**逐键翻译**。

### 层与入口

| 层 | 值 | 怎么进 | 怎么出 |
| --- | --- | --- | --- |
| `_BASE` shift 层（普通输出） | 0 | 默认层；右空格也回这层 | — |
| `_CTRL` ctrl 层 | 1 | 点按一次**左空格** | 点按右空格 |
| `_SYM` 符号层 | 2 | 点按一次 **F**（左手食指） | 敲一个键后自动退出（一次性层） |

- 层号里 `_SYM` 必须比 `_CTRL` 大：层查表从高往低找，这样「在 ctrl 层里点 F 进符号层」
  时，符号层才能盖住 ctrl 层在 J/K/L/; 上的方向键覆盖键。
- **esc 层**没有做成 QMK 的层：它就是「按住 Caps 位」，按住时给 ctrl+alt 两个修饰键
  （`MT(MOD_LCTL | MOD_LALT, KC_ESC)`），点按仍然是 Esc。
- Tab 同理：`MT(MOD_LALT | MOD_LSFT, KC_TAB)`，点按 Tab、按住 alt+shift；
  先按住 Shift 再点按 Tab 得到的就是 Shift+Tab（物理 Shift 一直在按着）。

> 📌 **符号层里两个分裂空格是 `XXXXXXX`（无行为）**，和上面「所有层的这两个键都进
> ctrl/shift 层」的说法有个例外：`_SYM` 是「敲一个键就退出」的一次性层，空格不是符号，
> 也不希望它在本层「强制跳去某个层」。要切层就敲完符号回上一层后，再点左/右空格。
> （`_BASE` 和 `_CTRL` 里两个空格仍是原来的左右双功能键。）

#### 两个分裂空格（ctrl 层的左空格是三态键）

设计里写着「左边的分裂空格被设置成 ctrl，右边的分裂空格被设置成 shift」，KLE 的图例也印证了：

| | shift 层面板 | ctrl 层面板 |
| --- | --- | --- |
| 左空格 | `→CTRL` | `CTRL` |
| 右空格 | `SHIFT` | `→SHIFT` |

**带箭头 = 切层，不带箭头 = 就是那个修饰键**。所以：

| 键 | 在哪层 | 点按 | 按住 |
| --- | --- | --- | --- |
| 左空格 | shift 层（及其它层） | 进 ctrl 层 | `Ctrl` |
| 左空格 | **ctrl 层** | **一记 `Ctrl` 单击** | **`Ctrl` + 临时回 shift 层** |
| 右空格 | shift 层 | 实打实的 `KC_RSFT`（点按 = 切中英文） | `Shift` |
| 右空格 | ctrl 层 | 回 shift 层 | `Shift` |
| 两个空格 | 符号层 | `XXXXXXX` 无行为 | 同左 |

**ctrl 层里按住左空格为什么要「临时回 shift 层」**：

ctrl 层在 `J/K/L/;` 上放的是 `←↓↑→`、在 `C/V` 上是 `Ctrl+C/V`。如果按住左空格（想拿 `Ctrl`）
时层不变，按 `J` 出来的就是 `Ctrl+↓` 而不是 `Ctrl+J`。所以按住期间把 ctrl 层**临时借走**：
`Ctrl` 仍然按着（走 `MT` 原生），但层查表落回 shift 层，`J` 恢复成字母。

| 操作 | 结果 |
| --- | --- |
| 按住左空格 + `J` | `Ctrl+J` |
| 按住左空格 + 按住右空格 + `J` | `Ctrl+Shift+J` |
| 只按住右空格 + `J` | `Shift+↓`（ctrl 层没被借走，`J` 在 ctrl 层就是 `↓`） |

松开左空格，ctrl 层原样还回来。

> ⚠ **时序要点**：这一招能成立，靠的是 `HOLD_ON_OTHER_KEY_PRESS_PER_KEY`。
> 按下另一个键时，QMK 会**先把左空格那一下按「按住」送进 `process_record_user`**
> （`layer_off(_CTRL)` 在这里执行），**之后**才去解析新键属于哪一层。
> 顺序反过来，`J` 就会被解析成 ctrl 层的方向键。
>
> ⚠ **另一个坑**：`layer_off(_CTRL)` 之后，QMK 会**拿着新层状态重新解析这颗键自己的 action**
> （`action.c` 的顺序：先跑 `process_record_user`，再 `store_or_get_action`）。
> 所以 **shift 层 `[5,3]` 那一格必须也是 `SPC_CTRL`** —— 如果写成 `XXXXXXX`，
> `MT` 的「按住」分支就没了，`Ctrl` 根本不会被注册。
>
> ✅ 这套时序有单元测试钉着：`tests/gk87_two_left_space/`（跑 `make test:gk87_two_left_space`），
> 5 个用例正好覆盖上面那张表。

#### F 用 `OSL`，不再用 tap-hold（重要）

早期版本把 F 做成 `LT(_FN, KC_NO)`（点按 = 符号层、按住 = 功能层）。**这个已废弃**，原因：

- tap-hold 必须等 `TAPPING_TERM`（200ms）才能判定，每次按 F 都有延迟；
- 更麻烦的是**抢跑**：左手刚按下 F、右手在 200ms 内敲了另一个键，QMK 就判成「按住」
  而不是点按 —— 明明想输符号，结果进了功能层。而左手食指按 F、右手同时快速敲别的键
  是很自然的打字节奏，冲突频繁。

现在只保留「跟随」，用 QMK 原生的 **`OSL(_SYM)`**（一次性符号层）：

- 按下 F 的**那一瞬间**就 `layer_on(_SYM)`，不存在等待；
- 敲任意一个键 → 那一键在符号层解析（输出符号）→ 层自动关，**回到刚刚的层**；
- **完全不涉及 tap-hold**，所以没有 `TAPPING_TERM` 延迟、也没有抢跑问题。

> ⚠ 两个行为细节：
>
> 1. **连点两下 F = 输出 `/`**。F 进了符号层后，再按一下 F 就是在符号层里解析了 ——
>    设计图里 F 位键帽左下写的就是 `/`（`_SYM[3,4] = KC_SLSH`），所以「F 再 F」
>    自然得到 `/`。**不要打开 `ONESHOT_TAP_TOGGLE`**，那会把双击变成「锁定/解锁层」，
>    `/` 就发不出来了（`config.h` 里有说明）。
> 2. 按住 F 不放时，**只有第一个键**享用符号层，之后层就关了（哪怕 F 还按着）。
>    要连打多个符号，每个符号前点一下 F 即可。

### KLE 图例 → 层

KLE 一个键最多 12 个图例槽位，这份设计用到 3 个：

| 槽位 | 显示位置 | 含义 |
| --- | --- | --- |
| 1 | 键帽**上排** | shift 层（`_BASE`）输出 |
| 6 | 键帽**左下** | 符号层（`_SYM`）输出 |

（依据是 KLE 里 shift 层图右下角的注「左下为符号层跟随键输出」。
曾经用过的槽位 8「右下 = 长按符号键输出」已随 `_FN` 一起废弃。）

「两层输出相同的键只画一份」对应到 QMK 有两种写法，`keymap.c` 里两种都在用：

| 写法 | 含义 |
| --- | --- |
| `_______`（`KC_TRNS`） | 透传 —— 这一格沿用下面层（shift 层）的输出 |
| `XXXXXXX`（`KC_NO`） | 空键 —— 这一格什么都不输出 |

### 两个非基础层是「排他」的：按设计图放行，其余静音

总规则一句话：**KLE 里那一层面板上画了图例的键 = 可用；图例空白 = `XXXXXXX`（无输出）**。
这样敲到没设计的键，就不会把下面 shift 层的字符漏出来（尤其是 `_CTRL` 里那些字母位）。

`_SYM` 的「可用范围」就是 shift 面板上**键帽左下**那圈小字图例。

`_CTRL`（ctrl 面板）放行的：

| 类别 | 键 |
| --- | --- |
| 字符类 | 数字行 `1`–`0`、`-`、`=`、`Backspace`（`Ctrl+1..0` 切标签页 / vim 计数前缀用）、`Enter` |
| 重映射 | 空格（物理 **E** 位）、退格（物理 **O** 位）、F = →符号、J/K/L/; = ←↓↑→、C/V = Ctrl+C/V |
| 层入口 | 左空格 = `Ctrl`（点按 = 一记 `Ctrl` 单击，按住 = `Ctrl` + 临时回 shift 层）、右空格 = 回 shift 层、右 Alt 位 = 空格 |
| 导航/修饰 | `Esc`、`Tab`、`F1`–`F12`、音量、`Ins`/`Home`/`End`/`PgUp`/`PgDn`、方向键、Shift/Ctrl/Alt/Win/Menu |
| **屏蔽** | 其余全部（各字母位、`` ` `` 位…）——敲了不出任何东西 |

各层实际状态（`qmk c2json` 核对过，透传/空键/明写 三者之和都是 87）：

| 层 | 透传 `_______` | 空键 `XXXXXXX` | 明写键码 |
| --- | --- | --- | --- |
| `_CTRL` | 50 | 23 | 14（Tab、物理 E=空格、物理 O=退格、Esc、F、←↓↑→、Ctrl+C/V、左右空格、右 Alt 位） |
| `_SYM` | 34 | 23 | 30（26 个符号 + Pin/Del/变亮/变暗） |

> ⚠ **符号层的两个分裂空格是 `XXXXXXX`（无行为）**：本层是一次性层、空格不是符号，
> 也不想让它「强制跳去某个层」。要切层就敲完符号回上一层后，再点左/右空格。

> ⚠ **想放开某个键**：把那一格的 `XXXXXXX` 改成 `_______`（变成透传、跟 shift 层一致）
> 或直接写成具体键码即可。三层每行结构都是 14 / 17 / 17 / 13 / 13 / 13，
> 改动时别打乱参数个数（`LAYOUT_all` 共 87 个）。

> 注：`_CTRL` 里 `1`–`0` / `-` / `=` / `Backspace` / `Enter` 都是 `_______` 透传，
> 所以它们输出的是 shift 层的值（数字、`-`、`=`、退格、回车）——这也是
> 「KLE 上同一格两层相同就只画一份」的写法。

### 字母重排（按设计原样实现）

| 物理行 | 物理键（左→右） | `_BASE` 输出 |
| --- | --- | --- |
| Q 行 | `Q W E R T Y U I O P [ ] \` | `E W ␣ F T ● Y U BS O ● P PgUp` |
| A 行 | `A S D F G H J K L ; '` | `A S D →符号 G ● H J K L I` |
| Z 行 | `Z X C V B N M , . /` | `Z X C V B Q N M R PgDn` |

（`●` = 该键输出为空 = `KC_NO`；`␣` = 空格；`BS` = 退格）

结果就是：空格在 **E** 键上、退格在 **O** 键上，物理 `Y` / `H` / `[` 三个键空着；
而 `\`、`/` 两个字符改到**符号层**取（分别在 A 键、F 键上）。

另有**物理右 Shift（`[4,12]`）也是空键 `XXXXXXX`**：KLE 的 shift / ctrl 两个面板都没给它
画图例（整个 KLE 里 `Shift` 只出现在左边的左 Shift 上），Shift 这个角色由**右空格**
（`[5,8]`）承担 —— 不是漏画，是设计里就空着。

### 可调项

| 想改什么 | 改哪 |
| --- | --- |
| 切换中英文 | 不用改：**右空格**在 shift 层上就是实打实的 `KC_RSFT`，点一下右 Shift 由输入法自己切 |
| 符号层 Esc 位发什么（默认字符串 `030828`） | `keymap.c` 的 `SYM_PIN` 分支 |
| 符号层里 ↑/↓ 干的活 | `keymap.c` 里 `_SYM` 的 `BL_UP` / `BL_DOWN` |
| ctrl/符号层里某个键想放开（不再屏蔽） | 把那一格的 `XXXXXXX` 改成 `_______` 或具体键码 |
| 一次性符号层的超时（默认 5 秒；删掉那个宏 = 永不超时） | `config.h` 的 `ONESHOT_TIMEOUT` |
| tap/hold 判定时间、`HOLD_ON_OTHER_KEY_PRESS_PER_KEY` | `config.h` 里的注释 |
| 旋钮（默认三层都是音量） | `keymap.c` 的 `encoder_map` |

### 符号层的取值

- **符号层 Esc 位（KLE 里的 “Pin”）**：发送字符串 `030828`。
- **符号层由 F 进入**，敲一个键（含符号、Pin、Del、背光）后自动退出。
- **符号层的 ↑ / ↓ = 背光亮度**：`BL_UP`（变亮）/ `BL_DOWN`（变暗）。
  本键盘是**单色 PWM 背光**（info.json 里只有 `backlight`，没有 `rgb_matrix`），
  所以只做了亮度，没有色调/饱和度那套；共 **20 级**（`backlight.levels = 20`）。
  - 全黑时按 ↑ 会自动把背光打开（`backlight_increase()` 会置 `enable = 1`）；
    按 ↓ 到 0 会自动关闭背光。
  - 没放 Toggle / Breathing / On / Off（你说只用调亮度）。
- **符号层 O 位 = `Del`**：`_BASE` 那里是退格，符号层改成 `Delete`。
- **符号层 F 位 = `/`**：这就是「连点两下 F 输出 `/`」的来源（F 先开层、F 再取符号）。
- **符号层一律按英文半角符号实现。** KLE 里写成中文标点的，一律换成对应的英文符号，
  例如弯引号 `’`（U+2019）→ 直引号 `'`（`KC_QUOT`）。其余符号本来就是半角，一对一。

### 还没定的（KLE 里没画 / 没写清）

- **底行中间的 `[5,5]`（info.json 的 `SPAC`）**：设计只画了两个 3.125u 分裂空格，
  这一位是整条 6.25u 空格的中间开关（装分裂空格时它正好落在分缝里），固件给了 `KC_SPC`。
- **底行 `[5,9]`（右 Alt 位）**：KLE 上写的是 `SP`，固件给了 `KC_SPC`。
- 符号层里 `[1,0]`（物理 `~` 那个键）KLE 两张面板都没画图例，固件沿用 `KC_GRV`；
  `_CTRL` 那一格按「空白 = 无输出」保持屏蔽，想放开把它改成 `_______` 即可。

### 编译（本机环境）

工具链和 qmk CLI 都不在 Windows PATH 里，必须走 MSYS2 的**登录 shell**：

```bash
# MSYS2 的 MINGW64 终端（或 VS Code 里的 “QMK MSYS2” 终端）
make skyloong/gk87/bl_pro/ansi:two
```

产物 `skyloong_gk87_bl_pro_ansi_two.bin`（约 32 KB）。

| 项 | 路径 |
| --- | --- |
| MSYS2 根 | `C:\Users\jomeu\msys64` |
| ARM/AVR 工具链 | `/opt/qmk/bin` |
| qmk CLI | `/opt/uv/tools/bin` |
| python3（Makefile 要用） | `/mingw64/bin` |
| make | `/usr/bin` |

`/etc/profile.d/qmk-uv-env.sh` 里已经把 `/opt/qmk/bin` 和 `/opt/uv/tools/bin` 加进 PATH。

**报 `arm-none-eabi-gcc: command not found`** → 当前 shell 没读登录 profile。用
`bash --login`，或显式导出：

```bash
export PATH=/opt/qmk/bin:/usr/bin:/opt/uv/tools/bin:/mingw64/bin:$PATH
```

**VS Code 任务**：`.vscode/tasks.json` 里有 4 个（编译 / 全量重编 / 烧录 / 打印布局）。
它们必须是 `"type": "process"` —— 写成 `"shell"` 时 VS Code 会把命令再套一层 Windows
shell（本机是 PowerShell），`bash.exe -lc '...'` 里的引号被解析坏，报
**「参数格式不正确 - -Command」**。

---

## 排障

### 1. shift 层的字母打不出来（J/K/L/; 变方向键、C/V 变 Ctrl+C/V）

**根因：基础层（default layer）被写成了 `_CTRL`。**

QMK 选层的规则**不是「最高层赢」**，而是 `layer_switch_get_layer()`
（`quantum/action_layer.c`）：

```c
layers = layer_state | default_layer_state;
/* 从高到低，找第一个在那个位置不是 KC_TRNS 的层 */
for (int8_t i = MAX_LAYER - 1; i >= 0; i--) {
    if (layers & ((layer_state_t)1 << i)) {
        action = action_for_key(i, key);
        if (action.code != ACTION_TRANSPARENT) return i;
    }
}
```

所以基础层一旦是 `_CTRL`，shift 层里所有写成 `_______`（透传）的位置都会落到 `_CTRL`
的覆盖键上：

| 物理键 | 实际得到 | 为什么 |
| --- | --- | --- |
| J / K / L / ; | ← / ↓ / ↑ / → | `_CTRL` 在这四个位置有方向键 |
| C / V | Ctrl+C / Ctrl+V | `_CTRL` 在这两个位置有 `LCTL(KC_C/V)` |
| H | 无输出 | `_CTRL` 那里是透传，透到底又碰上 `_BASE` 的 `KC_NO` |
| 右空格 | 点按无反应 | 被解析成 `_CTRL` 的 `SPC_SHIFT`，而它的点按就是「回 shift 层」，已经回不去了 |

**触发源：厂商 `bl_pro.c` 的 `process_record_kb()` 劫持了 `TO(0)` / `TO(1)`：**

```c
case TO(0): set_single_persistent_default_layer(0); return true;
case TO(1): set_single_persistent_default_layer(1); return true;
```

`set_single_persistent_default_layer()` 会**把基础层写进 EEPROM**，所以早期版本用
`TO(_CTRL)`（= `TO(1)`）做左空格时，一按就把基础层永久改成 `_CTRL`，重启不恢复、
刷固件也不恢复（固件里的值是启动时从 EEPROM 读的）。

**现在有两道防线（都在 `keymap.c` 里）：**

| 防线 | 位置 | 作用 |
| --- | --- | --- |
| ① 钉死基础层 | `default_layer_state_set_user()` 无条件 `return (layer_state_t)1 << _BASE` | `quantum_init()` 读完 EEPROM 一定会调 `default_layer_set()`，而它会回调到这里，所以**开机第一步**就把坏值纠正掉；之后任何代码（包括 `bl_pro.c`）也改不动 |
| ② 修 EEPROM | `keyboard_post_init_user()`：发现 `eeconfig_read_default_layer()` 不是 `_BASE` 就写正 | 只写一次，避免反复擦写；顺带让右上角屏幕的 WIN/MAC 图标刷新正确 |

切层也不再碰 `default_layer_state`：自定义的 `switch_layer()` 只做
`layer_clear()` + `layer_on()`。

> 💡 **顺手确认**：`bl_pro.c` 的 `default_layer_state_set_kb()` 按 `get_highest_layer(state)`
> 点亮右上角屏幕的 WIN（0）/ MAC（1）图标 —— 基础层被写坏时右上角会亮 MAC。

**已经中招怎么办**：刷一次新固件就行，EEPROM 里的坏值会在第一次开机时被改回 `_BASE`。



