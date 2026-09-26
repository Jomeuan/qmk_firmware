// Copyright 2026 jomeu
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

// ─────────────────────────────────────────────────────────────────────────────
// GK87「two」键位设计的固件实现
// ─────────────────────────────────────────────────────────────────────────────
//
// 唯一的设计源：doc/keyboard-layout-editor/skyloong-gk87-two.json（KLE）
// 设计文字说明：doc/readme.md
//
// 这个文件是那份 KLE 的**逐键翻译**，不含发明创造；KLE 里没画的一律标 TODO。
//
// ── 1. 层 ────────────────────────────────────────────────────────────────────
//
//   层号   名字    叫什么      怎么进                                  怎么出
//   ─────────────────────────────────────────────────────────────────────────
//   0      _BASE   shift 层    默认就在这层；右空格也回这层            —
//                  （普通输出）
//   1      _CTRL   ctrl 层     点按一次左空格                         点按右空格
//   2      _SYM    符号层      点按一次 F（左手食指）                 敲一个键后自动退出
//   3      _GAME   game 层     【在符号层里】按底行 [5,9]（“→GAME”）  按底行 [5,9]（“→CTRL”）
//                  （标准键位）                                        先回 ctrl 层，再点右空格回 shift
//
//   两个分裂空格都是「点按 / 按住」双功能键（见第 2 节）。
//
//   层号顺序有讲究：_SYM 必须**高于** _CTRL。
//   因为「在 ctrl 层里点按 F」也要能进符号层，而 ctrl 层在 J/K/L/; 上放了下左右上
//   的覆盖键；层查表是从高往低找，_SYM > _CTRL 才能让符号层盖住 ctrl 层的覆盖键。
//   （切层都是 switch_layer() 的独占领式，所以 _GAME 放最高只是为了不挡事。）
//
//   ⚠ 曾经还有一个 _FN「功能层」（按住 F 进入，放 Pin / Del / 背光），**现已取消**，
//   内容全部并入符号层 —— 原因见第 3 节。
//
// ── 2. 两个分裂空格（方案核心之一） ────────────────────────────────────────
//
//   设计里写着「左边的分裂空格被设置成 ctrl，右边的分裂空格被设置成 shift」，
//   又说「快按一次左/右空格进入 ctrl/shift 层」。KLE 里这两格的图例也印证了：
//
//     shift 层面板： 左空格 = 「→CTRL」（带箭头 = 切层）  右空格 = 「SHIFT」（不带箭头 = 就是 Shift 键）
//     ctrl  层面板： 左空格 = 「CTRL」                     右空格 = 「→SHIFT」
//
//   所以两个都是「点按 / 按住」双功能，其中左空格在 ctrl 层里还有第三个状态：
//
//     左空格 SPC_CTRL ：
//       shift 层（或其它层）：点按 = 进 ctrl 层        按住 = Ctrl 修饰键
//       ctrl 层：            点按 = 一记 Ctrl 单击     按住 = Ctrl + 临时回 shift 层
//       （ctrl 层面板上这一格写的就是光秃秃的「CTRL」、不带箭头 —— 它就是一颗 Ctrl 键。
//        按住那个「临时回 shift 层」是本 keymap 自己加的（KLE 画不出来）。）
//
//     右空格：
//       shift 层：就是实打实的 KC_RSFT（按住 Shift / 点按切中英文）
//       ctrl 层：不能用真 Shift（要先回 shift 层），所以用 SPC_SHIFT
//                （按住 Shift / 点按回 shift 层）
//       符号层：两个空格都是 XXXXXXX（无行为）—— 见 4.1 节末尾
//
//   为什么要「按住左空格 + 临时回 shift 层」：
//     ctrl 层在 J/K/L/; 上放的是 ←↓↑→、在 C/V 上是 Ctrl+C/V。如果按住左空格
//     （想拿 Ctrl）时层不变，按 J 出来的就是 Ctrl+↓ 而不是 Ctrl+J。
//     所以按住期间把 ctrl 层**临时借走**：Ctrl 仍然按着（走 MT 原生），但层查表
//     落回 shift 层，J 恢复成字母 → Ctrl+J；松开左空格，ctrl 层原样还回来。
//
//     · 按住左空格 + 按 J        → Ctrl+J
//     · 再同时按住右空格 + 按 J  → Ctrl+Shift+J（shift 层上右空格是实打实的 KC_RSFT）
//     · 只按住右空格 + 按 J      → Shift+↓（ctrl 层没被借走，J 在 ctrl 层就是 ↓）
//
//   ⚠ 时序要点：这件事能成立，靠的是 config.h 里那个 HOLD_ON_OTHER_KEY_PRESS_PER_KEY。
//   按下另一个键时，QMK 会**先把左空格那一下按「按住」送进 process_record_user**
//   （我们在这里 layer_off(_CTRL)），**之后**才去解析新键属于哪一层。
//   顺序反过来（先解析新键、后处理按住）的话，J 会被解析成 ctrl 层的方向键。
//
//   ⚠ 另一个关键：**点按进层，按住不进层**（指 shift 层上的常规用法）。
//   所以按住左空格再按 C/V/J/K/L 得到的是 Ctrl+C / Ctrl+V / Ctrl+J / Ctrl+K /
//   Ctrl+L（正常的修饰键组合），不会被 ctrl 层里 C/V 的 Ctrl+C、J/K/L/; 的方向键
//   给覆盖掉。想要 ctrl 层的方向键，就先把左空格「点」一下（松开），再按 J/K/L/;。
//
//   另一个关键：为了 Ctrl+C / Shift+A 这类组合**按下另一个键的瞬间**就生效
//   （而不是等 200ms），本 keymap 开了 HOLD_ON_OTHER_KEY_PRESS_PER_KEY，
//   只对这两个分裂空格生效，实现见文件底部的 get_hold_on_other_key_press()。
//
//   ⚠ 切层一律**不用 QMK 内建的 TO(n)**！
//   厂商键盘级代码 bl_pro.c 的 process_record_kb() 把 TO(0) 和 TO(1) 劫持成了
//   set_single_persistent_default_layer(0/1) —— 也就是「持久化切换基础层」。
//
//   ⚠⚠ 为什么这很致命：QMK 选层的规则**不是「最高层赢」**，而是
//      layer_switch_get_layer()（quantum/action_layer.c）：
//        layers = layer_state | default_layer_state;
//        从高到低，找第一个**在那个位置不是 KC_TRNS** 的层
//      基础层一旦被写成 _CTRL，shift 层里那些「______ 透传」的位置就会落到
//      _CTRL 的覆盖键上，症状：
//        · 物理 J/K/L/; → 方向键（_CTRL 在那里有 KC_LEFT/DOWN/UP/RGHT）
//        · 物理 C/V → Ctrl+C / Ctrl+V（_CTRL 在那里有 LCTL(KC_C) / LCTL(KC_V)）
//        · 物理 H → _BASE 那边本来就是 KC_NO，于是什么都打不出来
//        · 右空格被解析成 _CTRL 的 SPC_SHIFT，且点按走切层也回不去（default 层已经是 1）
//        · 重启不恢复，因为它是持久化的
//   两道防线：
//     1) 切层只用 layer_state（自定义 switch_layer()），不写 EEPROM，不再制造坏值；
//     2) default_layer_state_set_user() 无条件把基础层钉在 _BASE ——
//        quantum_init() 读完 EEPROM 一定会调 default_layer_set()，从而经过这个钩子，
//        所以**旧固件写坏的 EEPROM 在开机第一步就被纠正**；
//        keyboard_post_init_user() 再把 EEPROM 里的残留坏值写正（只写一次）。
//
// ── 3. F 用 OSL（一次性层），不用 LT（点按/按住） ───────────────────────────
//
//   早期版本把 F 做成 LT(_FN, KC_NO)：「点按 = 符号层，按住 = 功能层」。
//  **这个方案已废弃**，原因（实测）：
//     · tap-hold 必须等 TAPPING_TERM（200ms）才能判定是点按还是按住，所以每次按 F
//       都有延迟；
//     · 更麻烦的是「抢跑」：左手刚按下 F，右手在 200ms 内按了另一个键，
//       QMK 就会把 F 判成「按住」而不是点按 —— 明明想输出符号，结果进了功能层。
//       左手食指按 F、右手同时快速敲别的键是很自然的打字节奏，冲突频繁。
//
//   所以现在只保留「跟随」这一个特性，用 QMK 原生的 **OSL(_SYM)**（一次性符号层）：
//     · 按下 F 的**那一瞬间**就 layer_on(_SYM)，不存在等待；
//     · 敲任意一个键 -> 那一键在符号层解析（输出符号）-> 层自动关，回到刚刚的层；
//     · 完全不涉及 tap-hold，也就没有 TAPPING_TERM 延迟和抢跑问题。
//
//   ⚠ 两个重要的行为细节：
//
//   1) **连点两下 F = 输出 `/`**。
//      因为 F 进入符号层后，再按一下 F 就是在符号层里解析了 —— 而设计图（KLE）里
//      F 位键帽左下写的就是 `/`（`_SYM[3,4] = KC_SLSH`）。所以「F 再 F」自然得到
//      `/`，不需要额外写任何代码。
//      ⚠ 千万不要打开 `ONESHOT_TAP_TOGGLE` —— 那会把双击变成「锁定/解锁符号层」，
//        `/` 就发不出来了（config.h 里有详细说明）。
//
//   2) 按住 F 不放时，**只有第一个键**享用符号层，之后层就关了（哪怕 F 还按着）。
//      这是一次性层的固有语义。要连打多个符号，每个符号前点一下 F 即可。
//
// ── 4. KLE 图例怎么读（已用 KLE 内部模型逐键核对过） ─────────────────────────
//
//   KLE 把一个键的图例存在 12 个槽位里。**这份设计用三个**（合并面板的三行图例）：
//
//     槽位 1（下标 0）= 视觉**第 1 行** = shift 层（_BASE）的输出
//     槽位 7（下标 6）= 视觉**第 2 行** = ctrl 层（_CTRL）的输出
//     槽位 2（下标 1）= 视觉**第 3 行** = 符号层（_SYM）的输出
//
//   ⚠ 注意「视觉行」和「下标」**不是同顺序**：面板标题条写的是
//     “shift层 / 符号层 / …/ ctrl层”，也就是下标 0=shift、1=符号、6=ctrl；
//   而 KLE 把它们渲染成上→下 = 下标 0、6、1。核对时以下标为准（例：`C` 键上的
//   “(” 在下标 1、“Ctrl+v” 在下标 6，只有把 1 当符号、6 当 ctrl 才讲得通）。
//
//   （曾经用过的槽位 8「右下 = 长按符号键输出」已随 _FN 一起废弃，见第 3 节。）
//
//   KLE 的约定是「两层输出相同的键只画一份」。对应到 QMK 有两种写法，本文件都用：
//
//     _______  （KC_TRNS，透传）= 这一格沿用下面层的输出
//     XXXXXXX  （KC_NO，无键）    = 这一格什么都不输出
//
// ── 4.1 两个非基础层是「排他」的：按设计图放行，其余静音 ──────────────
//
//   总规则一句话：**KLE 那一层面板上画了图例的键 = 可用；图例空白 = XXXXXXX**。
//
//   这样敲到没设计的键不会把下面 shift 层的字符漏出来（尤其是 _CTRL 里那些
//   字母位）。_SYM 的「可用范围」就是 shift 面板上**键帽左下**那圈小字图例
//   （KLE 注释里写的「左下为符号层跟随键输出」）。
//
//   _CTRL 的可用范围（KLE 的 ctrl 面板）：
//
//     数字行 1-0 / `-` / `=` / Backspace   ← Ctrl+1..0 切标签页、vim 计数前缀
//     空格（物理 E 位）/ 退格（物理 O 位）
//     Tab / Esc（Caps 位）/ F = →符号 / J-K-L-; = ←↓↑→ / C-V = Ctrl+C-V / Enter
//     左空格 = 切到本层、右空格 = 回 shift 层、右 Alt 位 = 空格
//     导航与修饰键：Ins/Home/End/PgUp/PgDn、方向键、Shift/Ctrl/Alt/Win/Menu、
//                   F1-F12、音量
//
//   ⚠ 想放开某个键：把那一格的 XXXXXXX 改成 _______（透传）或写成具体键码。
//
//   ⚠ _SYM 里的两个分裂空格是 XXXXXXX（无行为）：本层是一次性层、空格不是符号，
//     不想让它「强制跳去某个层」。要切层就敲完符号回上一层再点空格。
//
// ── 5. 字母重排（设计者刻意做的：右手位整体向右挪了一位） ─────────────────────
//
//   物理键 -> _BASE 输出（按物理顺序读）：
//
//     Q W E R T Y U I O P [ ] \   ->   E W ␣ F T ● Y U BS O ● P PgUp
//     A S D F G H J K L ; '        ->   A S D →符号 G ● H J K L I
//     Z X C V B N M , . /          ->   Z X C V B Q N M R PgDn
//
//   几个显眼的后果（都是设计里写死的，不是笔误）：
//     · 空格在 E 键上（两个分裂空格被拿去当层入口了）
//     · BS（退格）在 O 键上，原来的退格位没有被用
//     · 物理 Y / H / [ 三个键输出为空（KC_NO）
//     · 物理**右 Shift**（[4,12]）也是空键 XXXXXXX：KLE 的 shift / ctrl 两个面板
//       都没给它画图例（整个 KLE 里 Shift 只出现在左边的左 Shift 上）。
//       Shift 这个角色由**右空格**（[5,8]）承担 —— 不是漏画，是设计里就空着。
//     · 物理 \ 键 = PgUp，物理 / 键 = PgDn；「\」「/」两个字符改到符号层取
//
// ── 6. game 层：游戏用的「标准键位」 ────────────────────────────────────────
//
//   为什么要单独一层：shift 层的字母是重排过的（右手整体右移一位、空格在 E 键上…），
//   而绝大多数游戏都假定标准 QWERTY，照 shift 层打游戏会很别扭。所以单开一层。
//
//   入口／出口（都在 KLE 底行那排 1.25u 键的**第一个**，下标 [5,9]）：
//     · 符号层里它是「→GAME」：点一下 F（进符号层）再按它 → 进 game 层
//     · game 层里它是「→CTRL」：进了 game 后，按它 → ctrl 层，再点按右空格 → 回 shift 层
//   （即 QMK 层号是**独占领**式的，四个层同时只有一个生效，见 switch_layer()。）
//
//   与 shift 层的全部差别（其余 80 格完全相同）：
//     · 物理 **Esc 位 = Caps Lock**、物理 **Caps 位 = Esc** —— 两者互换，
//       把游戏里最常用的 Esc 挪到右手更好按的位置。（KLE 的 game 面板上就写着 Caps/Esc）
//     · 物理 **E 位 = `Q`**、物理 **R 位 = `R`** —— shift 层这两格是「空格」「F」，
//       游戏层把它们换成左手缺的两个字母（shift 层里 Q 在 N 键、R 在 . 键上）。
//       所以本层 Q/R 各出现两次，是故意的（为游戏手感）。
//     · 物理 **F 位 = `F`** —— shift 层那里是「→符号」入口，但游戏需要真的 F 键。
//     · 两个分裂空格 = **普通空格 / 普通右 Shift**，不再兼职切层。
//     · 底行 [5,9] = 「→CTRL」（shift 层那里是空格）、[5,11]（Menu 位）无输出。
//     · 物理 Y / H / [ / 右 Shift 四格仍然无输出（和 shift 层一致）。
//
//   ⚠ 本层是**显式写全 87 格**的（不像 _CTRL/_SYM 靠透传），因为它是一套独立的键位表，
//   不想受 shift 层的重排影响；以后 shift 层改了字母，本层不会跟着变。
//
// ─────────────────────────────────────────────────────────────────────────────

#include QMK_KEYBOARD_H

enum layer_names {
    _BASE = 0, // ② shift 层：普通输出（字母已重排）
    _CTRL,     // ① ctrl 层：带修饰键的操作 + vim 式移动键
    _SYM,      // ④ 符号层：一次性，点按 F 进入
    _GAME,     // ③ game 层：打游戏用的标准键位，从符号层 [5,9] 进
};

enum custom_keycodes {
    SYM_PIN = QK_USER, // 符号层 Esc 位（KLE 里的 “Pin”）：发送字符串 "030828"
    SYM_GAME,          // 符号层 [5,9]（KLE 里的 “→GAME”）：切到 game 层
    GAME_CTRL,         // game 层 [5,9]（KLE 里的 “→CTRL”）：切回 ctrl 层
};

// ── 组合键（tap-hold） ───────────────────────────────────────────────────────

// 两个分裂空格：按住 = 修饰键，点按 = 切层。
//
// KC_NO 只是「点按」分支的占位键码：点按被 process_record_user 拦下来改成切层动作，
// 「按住」分支根本用不到这个 tap 键码（直接走 MT 原生的 register_mods/unregister_mods）。
// MT 的 tap 键码只能占 8 位基础键码，写不下层操作，所以才这么搭。
//
// ⚠ 切层动作走自定义的 switch_layer()，**不能**用 QMK 内建的 TO(n)：
// 厂商 bl_pro.c 把 TO(0)/TO(1) 劫持成了「持久化切换基础层」，会写坏 EEPROM
// （详见文件开头的说明）。
#define SPC_CTRL  MT(MOD_LCTL, KC_NO) // 左空格：按住 Ctrl，点按 → ctrl 层
#define SPC_SHIFT MT(MOD_LSFT, KC_NO) // 右空格（ctrl/符号层上）：按住 Shift，点按 → shift 层

// Caps 位（原 Esc 位）：点按 = Esc，按住 = ctrl+alt 修饰键。
// 设计里的「③ esc 层」就是这么实现的 —— 它是修饰键，不是 QMK 的层。
#define ESC_CAG MT(MOD_LCTL | MOD_LALT, KC_ESC)

// Tab：点按 = Tab，按住 = alt+shift 修饰键。
// 先按住别的修饰键（例如物理 Shift）再点按 Tab，输出的就是 Shift+Tab ——
// 真实 Shift 一直按着，QMK 把 Tab 判成点按后发 KC_TAB，系统看到的就是 Shift+Tab。
#define TAB_AS MT(MOD_LALT | MOD_LSFT, KC_TAB)

// 左手食指 F：一次性符号层（OSL）。
// **不再是 tap-hold**：按下即激活，没有任何等待；敲完一个键自动退出。
// 曾经的「按住 = 功能层」已取消，原因见文件开头第 3 节。
#define SYM_OSL OSL(_SYM)

// ── 层 ───────────────────────────────────────────────────────────────────────
//
// LAYOUT_all 共 87 个参数，行结构 14 / 17 / 17 / 13 / 13 / 13。
// 底行第 5 个参数是 [5,5]：设计里只画了「双 3.125u 空格」两个键，这个中间开关没画，
// 它其实就是厂商 info.json 里的 SPAC —— 「整条 6.25u 空格」的中间那一位，
// 不装分裂空格时才露出来（装了分裂空格它正好在分缝里，基本按不到），所以给普通空格。

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // ② shift 层 = _BASE（默认层）。KLE 里 shift 层那张图的「上排」图例。
    [_BASE] = LAYOUT_all(
        KC_ESC,     KC_F1,    KC_F2,    KC_F3,   KC_F4,      KC_F5,    KC_F6,    KC_F7,    KC_F8,     KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_MUTE,
        KC_GRV,     KC_1,     KC_2,     KC_3,    KC_4,       KC_5,     KC_6,     KC_7,     KC_8,      KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,   KC_INS,   KC_HOME,  KC_PGUP,
        TAB_AS,     KC_E,     KC_W,     KC_SPC,  KC_F,       KC_T,     KC_NO,    KC_Y,     KC_U,      KC_BSPC,  KC_O,     KC_NO,    KC_P,     KC_PGUP,   KC_DEL,   KC_END,   KC_PGDN,
        ESC_CAG,    KC_A,     KC_S,     KC_D,    SYM_OSL,    KC_G,     KC_NO,    KC_H,     KC_J,      KC_K,     KC_L,     KC_I,               KC_ENT,
        KC_LSFT,    KC_Z,     KC_X,     KC_C,    KC_V,       KC_B,     KC_Q,     KC_N,     KC_M,      KC_R,     KC_PGDN,                      XXXXXXX,             KC_UP,
        KC_LCTL,    KC_LGUI,  KC_LALT,           SPC_CTRL,   KC_SPC,             KC_RSFT,             KC_SPC,    KC_NO,    KC_APP,             KC_RCTL,   KC_LEFT,  KC_DOWN,  KC_RGHT
    ),

    // ① ctrl 层：点按左空格进入（KLE 里 ctrl 层那张图）。
    //
    // ⚠ 排他层（见文件开头 4.1 节）：**ctrl 面板上图例是空白的位置一律 XXXXXXX**，
    // 否则会透传到底下的 _BASE，把 shift 层的字符漏出来。
    //
    // 放行（ctrl 面板上画了图例的）：
    //   · 数字行 1-0 / `-` / `=` / Backspace —— 给 Ctrl+1..0 切标签页、vim 的计数前缀用
    //   · 空格（物理 E 位，新增）、退格（物理 O 位，新增）
    //   · Tab / Esc（Caps 位）/ F = →符号 / J-K-L-; = ←↓↑→ / C-V = Ctrl+C-V / Enter
    //   · 左空格 = CTRL（见第 2 节：点按 = 一记 Ctrl 单击，按住 = Ctrl + 临时回 shift 层）、
    //     右空格 = →SHIFT（点按回 shift 层）、右 Alt 位 = 空格
    //   · 导航与修饰键：Ins/Home/End/PgUp/PgDn、方向键、Shift/Ctrl/Alt/Win/Menu、F1-F12、音量
    [_CTRL] = LAYOUT_all(
        _______,    _______,  _______,  _______, _______,    _______,  _______,  _______,  _______,   _______,  _______,  _______,  _______,  _______,
        XXXXXXX,    _______,  _______,  _______, _______,    _______,  _______,  _______,  _______,   _______,  _______,  _______,  _______,   _______,   _______,  _______,  _______,
        TAB_AS,     XXXXXXX,  XXXXXXX,  KC_SPC,  XXXXXXX,    XXXXXXX,  _______,  XXXXXXX,  XXXXXXX,   KC_BSPC,  XXXXXXX,  _______,  XXXXXXX,   _______,   XXXXXXX,  _______,  _______,
        ESC_CAG,    XXXXXXX,  XXXXXXX,  XXXXXXX, SYM_OSL,    XXXXXXX,  _______,  KC_LEFT,  KC_DOWN,   KC_UP,    KC_RGHT,  XXXXXXX,            _______,
        _______,    XXXXXXX,  XXXXXXX,  LCTL(KC_C), LCTL(KC_V), XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, _______,                      _______,             _______,
        _______,    _______,  _______,           SPC_CTRL,   XXXXXXX,            SPC_SHIFT,           KC_SPC,    _______,  _______,            _______,   _______,  _______,  _______
    ),

    // ④ 符号层：点按 F 进入的一次性层，敲一个键后自动退出。
    // KLE 里 shift 层面板上**键帽左下**那圈小字图例（「左下为符号层跟随键输出」）。
    //
    // ⚠ 排他层：设计里没有符号的键一律 XXXXXXX（不透传，
    // 否则点一下 F 再随手敲个字母，出来的会是那个字母而不是符号）。
    // 透传保留的只有导航／修饰键（Home/End/PgUp/PgDn、方向键、Shift、Ctrl…）。
    //
    // 本层里四个「非符号」的功能键（原来是 _FN 功能层的，现已吞并进来）：
    //   Esc 位 [3,0] = Pin（字符串 "030828"）
    //   O   位 [2,9] = Del（_BASE 那里是退格，本层改成 Delete）
    //   ↑   位 [4,14] = 背光变亮（BL_UP）
    //   ↓   位 [5,14] = 背光变暗（BL_DOWN）
    //
    // ⚠ F 位 [3,4] = `/`（KC_SLSH）：它是进入本层的那把钥匙，所以「点一下 F、再点一下 F」
    //   就是输出 `/`（先开层、第二次在本层解析）。这是设计图里写明的，别改成 XXXXXXX。
    //
    // ⚠ 两个空格在**本层是 XXXXXXX（无行为）**，不切层、不出空格。
    //   原因：`_SYM` 是「敲一个键就退出」的一次性层，空格本身不是符号；
    //   而且不想在本层「强制跳去某个层」。想换层就敲完这个符号后自然回上一层，
    //   再点左/右空格切 ctrl / shift。
    // Q 行符号（左→右数物理键）：
    //   物理 Q = `~`、物理 W = `` ` ``、物理 R = `=`、物理 T = `:`、
    //   物理 I = `_`（**紧挨着它的右边就是 BS 键**，也就是物理 O）、
    //   物理 O = `Del`、物理 ] = `+`
    //   ⚠ 物理 U / Y / P / [ / \ 在这一行没有符号（U 和 [ 底下是 KC_NO）。
    [_SYM] = LAYOUT_all(
        _______,    _______,  _______,  _______, _______,    _______,  _______,  _______,  _______,   _______,  _______,  _______,  _______,  _______,
        XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX, XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,   XXXXXXX,   XXXXXXX,  _______,  _______,
        _______,    KC_TILD,  KC_GRV,   XXXXXXX, KC_EQL,     KC_COLN,  _______,  XXXXXXX,  KC_UNDS,   KC_DEL,   XXXXXXX,  _______,  KC_PLUS,   _______,   XXXXXXX,  _______,  _______,
        SYM_PIN,    KC_BSLS,  KC_MINS,  KC_LT,   KC_SLSH,    KC_SCLN,  _______,  KC_HOME,  KC_COMM,   KC_DOT,   KC_DQUO,  KC_END,             XXXXXXX,
        _______,    KC_LBRC,  KC_RBRC,  KC_LPRN, KC_RPRN,    KC_GT,    KC_QUES,  KC_LCBR,  KC_RCBR,   KC_PIPE,  KC_QUOT,                      _______,             BL_UP,
        _______,    _______,  _______,           XXXXXXX,    XXXXXXX,            XXXXXXX,             SYM_GAME,  _______,  _______,            _______,   _______,  BL_DOWN,  _______
    ),

    // ③ game 层：游戏用的「标准键位」，**显式写全 87 格**（设计说明见文件开头第 6 节）。
    //
    // 入口：符号层（点按 F）里按 [5,9]；出口：本层 [5,9] = →CTRL → ctrl 层 → 点右空格回 shift。
    // 与 shift 层的差别只有：Esc/Caps 互换、物理 E 位=Q、物理 R 位=R、物理 F 位=F、
    // 两个分裂空格变回普通空格/右 Shift、[5,9]=→CTRL、[5,11] 无输出。
    // （第 1 行第 14 格是 KLE 上写着 “Screen” 的屏幕位，跟 shift 层一样用透传 = Mute。）
    [_GAME] = LAYOUT_all(
        KC_CAPS,    KC_F1,    KC_F2,    KC_F3,   KC_F4,      KC_F5,    KC_F6,    KC_F7,    KC_F8,     KC_F9,    KC_F10,   KC_F11,   KC_F12,   _______,
        KC_GRV,     KC_1,     KC_2,     KC_3,    KC_4,       KC_5,     KC_6,     KC_7,     KC_8,      KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,   KC_INS,   KC_HOME,  KC_PGUP,
        KC_TAB,     KC_E,     KC_W,     KC_Q,    KC_R,       KC_T,     KC_NO,    KC_Y,     KC_U,      KC_BSPC,  KC_O,     KC_NO,    KC_P,     KC_PGUP,   KC_DEL,   KC_END,   KC_PGDN,
        KC_ESC,     KC_A,     KC_S,     KC_D,    KC_F,       KC_G,     KC_NO,    KC_H,     KC_J,      KC_K,     KC_L,     KC_I,               KC_ENT,
        KC_LSFT,    KC_Z,     KC_X,     KC_C,    KC_V,       KC_B,     KC_Q,     KC_N,     KC_M,      KC_R,     KC_PGDN,                      KC_NO,               KC_UP,
        KC_LCTL,    KC_LGUI,  KC_LALT,           KC_SPC,     KC_SPC,             KC_RSFT,             GAME_CTRL, KC_NO,    KC_NO,              KC_RCTL,   KC_LEFT,  KC_DOWN,  KC_RGHT
    )
};

// ── 切层 ─────────────────────────────────────────────────────────────────────
//
// 只动 layer_state（覆盖层），**不碰 default_layer_state、不写 EEPROM**。
// 先 layer_clear() 再 layer_on()，保证不会残留上一次的层位（从 _CTRL 出来时把
// 别的层也一并清掉）。回到 _BASE 只要 layer_clear() —— 基础层永远在线，
// 不需要把它写进 layer_state。
static void switch_layer(uint8_t layer) {
    layer_clear();
    if (layer != _BASE) {
        layer_on(layer);
    }
}

// ── 左空格的两个临时状态（只由下面 SPC_CTRL 那个分支读写）───────────────────
//
// spc_ctrl_lctl_down：在 ctrl 层里点按左空格时，我们替它按下的那记 Ctrl 是否还按着。
//                     按下时 register、松开时 unregister，成对出现，不会漏抬。
// spc_ctrl_tmp_base ：在 ctrl 层里按住左空格期间，是否已经把层临时让给了 shift 层。
//                     ⚠ 松开时必须看这个标记，**不能**看 layer_state_is(_CTRL) ——
//                     按住期间 _CTRL 已经被关掉了，那时它就是 false。
static bool spc_ctrl_lctl_down = false;
static bool spc_ctrl_tmp_base  = false;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        // ── 左空格：三态键 ─────────────────────────────────────────────
        // ① shift 层上点按 = 进 ctrl 层；② ctrl 层上点按 = 一记 Ctrl 单击；
        // ③ ctrl 层上按住 = 保持 Ctrl + 临时把层让回 shift 层。详见文件开头第 2 节。
        //
        // 切层用 switch_layer() 而不是 TO(_CTRL)，原因见文件开头对 bl_pro.c 劫持
        // TO(0)/TO(1) 的说明。switch_layer() 只动 layer_state，不碰 default_layer_state。
        case SPC_CTRL:
            if (record->tap.count > 0) {
                // ---- 点按（QMK 判定为 tap；按下和松开两个事件都会送进来）----
                if (record->event.pressed) {
                    if (layer_state_is(_CTRL)) {
                        // 【ctrl 层上】这一格就是一颗普通 Ctrl 键，点按 = 一记 Ctrl
                        // 单击：按下 register、松开 unregister（成对，不会漏抬）。
                        register_code(KC_LCTL);
                        spc_ctrl_lctl_down = true;
                    } else {
                        // 【shift 层／其它层上】点按 = 进 ctrl 层
                        switch_layer(_CTRL);
                    }
                } else if (spc_ctrl_lctl_down) {
                    unregister_code(KC_LCTL);
                    spc_ctrl_lctl_down = false;
                }
                return false; // 屏蔽 MT 的点按占位（那个 KC_NO）
            }
            // ---- 按住 ----
            if (record->event.pressed) {
                // ⚠ 每次按下先把旧标记清掉：万一某次「松开」事件没送到这里
                // （比如 USB 状态变化时 clear_keyboard() 并不派发松开事件），
                // 脏标记会让**下一次**在 shift 层上松开左空格时凭空打开 ctrl 层。
                spc_ctrl_tmp_base = false;
                if (layer_state_is(_CTRL)) {
                    // 【ctrl 层上按住】临时把 ctrl 层借走，让层查表落回 shift 层，
                    // 这样再接 J/K/L/; 得到的是字母（Ctrl+J）而不是本层的方向键
                    // （Ctrl+↓）。Ctrl 修饰键本身仍由 MT 原生逻辑保持按下。
                    //
                    // ⚠ 借走之后，QMK 会**拿着新层状态重新解析这颗键自己的 action**
                    // （action.c 的顺序：先跑 process_record_user，再 store_or_get_action），
                    // 所以 shift 层那一格必须也是 SPC_CTRL —— 否则 MT 的「按住」分支
                    // 就没了，Ctrl 根本不会被注册。_BASE[5,3] 正是 SPC_CTRL。
                    layer_off(_CTRL);
                    spc_ctrl_tmp_base = true;
                }
            } else if (spc_ctrl_tmp_base) {
                // 【松开】把 ctrl 层原样还回来。
                layer_on(_CTRL);
                spc_ctrl_tmp_base = false;
            }
            // 走 MT 原生的 register_mods(unregister_mods)：按住期间就是 Ctrl。
            return true;

        // ── 右空格（在 ctrl / 符号层上）：按住 = Shift，点按 = 回 shift 层 ──
        // shift 层自己那一格是实打实的 KC_RSFT（右 Shift）：按住能当修饰键用，
        // 点按就是输入法要的「按一下 Shift 切中英文」，所以不用走这个键码。
        case SPC_SHIFT:
            if (record->tap.count > 0) {
                if (record->event.pressed) {
                    switch_layer(_BASE);
                    reset_oneshot_layer(); // 顺手清掉可能还挂着的一次性符号层状态
                }
                return false; // 屏蔽 MT 的点按占位（那个 KC_NO）
            }
            // ---- 按住 ----
            // 走 MT 原生：按住期间就是 Shift，可以 Shift+方向键 选字。
            return true;

        // ── 符号层 Esc 位（KLE 里的 “Pin”） ──────────────────────────────────
        // 发送字符串 "030828"。想改成别的字符/长度就改下面这行。
        case SYM_PIN:
            if (record->event.pressed) {
                SEND_STRING("030828");
            }
            return false;

        // ── 层开关（都走自定义 switch_layer()，不碰 default_layer_state）────────
        // 符号层 [5,9] = →GAME：从符号层进 game 层
        case SYM_GAME:
            if (record->event.pressed) {
                switch_layer(_GAME);
            }
            return false;

        // game 层 [5,9] = →CTRL：出 game 层到 ctrl 层（再点右空格就回 shift 层）
        case GAME_CTRL:
            if (record->event.pressed) {
                switch_layer(_CTRL);
            }
            return false;
    }
    return true;
}
// ── 让分裂空格的修饰键「立即」生效 ────────────────────────────────────────────

// 默认情况下 MT 组合键要等 TAPPING_TERM（200ms）才知道是按住，所以「按住左空格马上
// 按 C」会被当点按、输出普通的 c。这两个分裂空格的主用途就是 Ctrl+C / Ctrl+V 和
// Shift+字母，等 200ms 没法用，所以对它们开 HOLD_ON_OTHER_KEY_PRESS_PER_KEY：
// 只要在按住期间按下了别的键，立刻当「按住」处理。
//
// 只对这两个键生效。F（一次性符号层）用的是 OSL，**根本不是 tap-hold**，不在这里面，
// 所以它没有任何延迟（这也是取消 LT 的原因）。
// 副作用：想进 ctrl 层必须先松开左空格（这就是设计里写的「快按」）。
#if defined(HOLD_ON_OTHER_KEY_PRESS_PER_KEY)
bool get_hold_on_other_key_press(uint16_t keycode, keyrecord_t *record) {
    return keycode == SPC_CTRL || keycode == SPC_SHIFT;
}
#endif

// ── 启动纠错 ─────────────────────────────────────────────────────────────────

// 第一道防线：把「基础层」钉死在 _BASE。
//
// quantum_init()（quantum/keyboard.c）会在开机时用 EEPROM 里存的值调
// default_layer_set()，而 default_layer_set() 最终会回调到这里
// （bl_pro.c 的 default_layer_state_set_kb() 就是转调它）。
// 所以无条件返回 _BASE，基础层就再也改不动了：无论是 bl_pro.c 的 TO(0)/TO(1)
// 劫持，还是旧固件已经写坏的 EEPROM 值，**开机第一步就会被纠正**。
layer_state_t default_layer_state_set_user(layer_state_t state) {
    (void)state;
    return (layer_state_t)1 << _BASE;
}

// 第二道防线：把 EEPROM 里的残留坏值写正（只在确实不对时写，避免反复擦写）。
// 顺带让 bl_pro.c 的 default_layer_state_set_kb() 再跑一次 —— 它在上面那次
// default_layer_set() 里读到的是坏值，会把右上角屏幕点亮成 MAC 图标。
void keyboard_post_init_user(void) {
    if (eeconfig_read_default_layer() != ((layer_state_t)1 << _BASE)) {
        set_single_persistent_default_layer(_BASE);
    }
}

// 右上角旋钮（encoder）。设计里没有规定各层旋钮干什么，沿用厂商 default 的「音量」。
// ⚠ 这个数组的长度必须和 keymaps[] 的层数一致（keymap_introspection.c 里有静态断言），
//    所以加一层就**必须**在这里也加一行，否则编译不过。
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [_CTRL] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [_SYM]  = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [_GAME] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
};
#endif
