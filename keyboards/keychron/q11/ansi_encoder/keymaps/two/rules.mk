# keymap「two」：按 KLE 1da15b6b07c6f42a59b51b4528fc9481 重写。
#
# 只需要旋钮映射（两个旋钮）。
# ⚠ encoder_map 的行数必须和 keymaps[] 的层数一致（core 里有 STATIC_ASSERT），加层要记得补。
ENCODER_MAP_ENABLE = yes

# Shift+Backspace = Delete
KEY_OVERRIDE_ENABLE = yes

# TextStream：主机通过 Raw HID 把文本灌进键盘 RAM 缓冲（32 B/报告）。
# ⚠ 与 VIA 互斥（两者抢 raw_hid_receive），本 keymap 不使用 VIA。
RAW_ENABLE = yes

# TextStream 的实现文件（keymap 目录下的额外 .c 需要显式加入；
# build_keyboard.mk 已把 KEYMAP_PATH 加进 VPATH，所以写相对名即可）。
SRC += textstream.c