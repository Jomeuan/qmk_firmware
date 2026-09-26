# keymap「two」：GK87 的层设计实现。
#
# 只需要旋钮映射（右上角那个旋钮）。旋钮在三层里都是音量，见 keymap.c 的 encoder_map。
ENCODER_MAP_ENABLE = yes

# 拨码开关（DIP）暂时不启用：厂商 default keymap 用它切 Windows/Mac 两个基础层，
# 这套设计不用 Mac，也没有第二套基础层，所以拨码开关目前不起作用。
# DIP_SWITCH_MAP_ENABLE = yes
