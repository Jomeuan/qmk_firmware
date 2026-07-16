/* Copyright 2023 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

enum layers{
    CTRL_L
    ,INPT_L
    ,GAME_L
    ,CESC_L
    ,SYB1_L
};

enum my_keycode{
    MY_GOPM = SAFE_RANGE
    ,MY_BSPC
    ,MY_SSFT
    ,MY_CT
    ,MY_CC
    ,MY_CV
    ,MY_CX
    ,MY_CZ
    ,MY_CF
    ,MY_PREW
    ,MY_TEST
    ,MY_BRTH
    // symbol
    ,MY_TILD
    ,MY_LPRN
    ,MY_RPRN
    ,MY_UNDS
    ,MY_PLUS
    ,MY_LCBR
    ,MY_RCBR
    ,MY_PIPE
    ,MY_COLN
    ,MY_DQUO
    ,MY_LABK
    ,MY_RABK
    ,MY_QUES
};

// Tap Dance declarations
enum {
    TD_SYBL
    ,TD_RSFT
    ,TD_LCTL
};

#define KC_TASK LGUI(KC_TAB)
#define KC_FLXP LGUI(KC_E)
#define MY_SNIP LCA(KC_S)
#define TO_INPT TO(INPT_L)
#define TO_CTRL TO(CTRL_L)

#define MY_RSFT RSFT_T(TO_INPT)
#define MY_LCTL LCTL_T(TO_CTRL)

#define MY_CESC LT(CESC_L,KC_ESC)

#define MY_SYBL TD(TD_SYBL)
#define MY_SYB1 OSL(SYB1_L)

#define CL_CTL LM(INPT_L,MOD_MASK_CTRL)
#define CL_ALT LM(INPT_L,MOD_MASK_ALT)
#define CL_WIN LM(INPT_L,MOD_MASK_GUI)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [CTRL_L] = LAYOUT_91_ansi(
        KC_MUTE,    XXXXXXX,  KC_F1,     KC_F2,     KC_F3,    KC_F4,    KC_F5,    KC_F6,       KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_INS,   KC_DEL,   KC_MUTE,
        KC_C,       XXXXXXX,  KC_1,      KC_2,      KC_3,     KC_4,     KC_5,     KC_6,        KC_7,     KC_8,     KC_9,     KC_0,     XXXXXXX,  XXXXXXX,  KC_BSPC,            KC_PGUP,
        XXXXXXX,    KC_TAB,   XXXXXXX,   C(KC_RGHT),KC_SPC,   C(KC_F),  C(KC_T),  XXXXXXX,     XXXXXXX,  LCA(KC_L),KC_BSPC,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_PGUP,            KC_PGDN,
        XXXXXXX,    MY_CESC,  XXXXXXX,   MS_WHLU,   MS_WHLD,  MY_SYB1,  XXXXXXX,  XXXXXXX,     KC_LEFT,  KC_DOWN,  KC_UP,    KC_RGHT,  XXXXXXX,            KC_ENT,             KC_HOME,
        XXXXXXX,    KC_LSFT,             MY_CZ,     MY_CX,    MY_CC,    MY_CV,    C(KC_LEFT),  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,            KC_PGDN,  KC_UP,
        XXXXXXX,    XXXXXXX,  XXXXXXX,   CL_WIN,    CL_ALT,             CL_CTL,                          MY_RSFT,            CL_ALT,   XXXXXXX,  CL_CTL,   KC_LEFT,  KC_DOWN,  KC_RGHT),

    [INPT_L] = LAYOUT_91_ansi(
        _______,    _______,  _______,   _______,   _______,  _______,  _______,  _______,     _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        KC_I,       _______,  _______,   _______,   _______,  _______,  _______,  _______,     _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        XXXXXXX,    _______,  KC_E,      KC_W,      KC_SPC,   KC_F,     KC_T,     XXXXXXX,     KC_Y,     KC_U,     _______,  KC_O,     KC_3,     KC_P,     _______,            _______,
        XXXXXXX,    _______,  KC_A,      KC_S,      KC_D,     MY_SYB1,  KC_G,     XXXXXXX,     KC_H,     KC_J,     KC_K,     KC_L,     KC_I,               _______,            _______,
        XXXXXXX,    _______,             KC_Z,      KC_X,     KC_C,     KC_V,     KC_B,        KC_Q,     KC_N,     KC_M,     KC_R,     KC_2,               _______,  _______,
        XXXXXXX,    _______,  _______,   _______,   _______,            MY_LCTL,                         KC_RSFT,            _______,  _______,  _______,  _______,  _______,  _______),

    [GAME_L] = LAYOUT_91_ansi(
        KC_MUTE,    KC_ESC,   KC_F1,     KC_F2,     KC_F3,    KC_F4,    KC_F5,    KC_F6,       KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_INS,   KC_DEL,   KC_MUTE,
        KC_G,       KC_GRV,   KC_1,      KC_2,      KC_3,     KC_4,     KC_5,     KC_6,        KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,
        XXXXXXX,    KC_TAB,   KC_Q,      KC_W,      KC_E,     KC_R,     KC_T,     XXXXXXX,     _______,  _______,  XXXXXXX,  _______,  XXXXXXX,  KC_RBRC,  KC_BSLS,            KC_PGDN,
        XXXXXXX,    MY_CESC,  KC_A,      KC_S,      KC_D,     KC_F,     KC_G,     XXXXXXX,     _______,  _______,  _______,  _______,  _______,            KC_ENT,             KC_HOME,
        XXXXXXX,    KC_LSFT,             KC_Z,      KC_X,     KC_C,     KC_V,     _______,     _______,  _______,  _______,  _______,  _______,            KC_RSFT,  KC_UP,
        XXXXXXX,    KC_LCTL,  KC_LWIN,   XXXXXXX,   KC_LALT,            KC_SPC,                          KC_SPC,             KC_RALT,  XXXXXXX,  KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [CESC_L] = LAYOUT_91_ansi(
        XXXXXXX,    XXXXXXX,  XXXXXXX,   XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,     XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        KC_E,       XXXXXXX,  XXXXXXX,   XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,     XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,            XXXXXXX,
        XXXXXXX,    RM_TOGG,  MY_TEST,   XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,     XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,            XXXXXXX,
        QK_BOOT,    XXXXXXX,  XXXXXXX,   MY_SNIP,   XXXXXXX,  MY_BRTH,  XXXXXXX,  XXXXXXX,     G(KC_LEFT),G(KC_DOWN),G(KC_UP),G(KC_RGHT),XXXXXXX,          XXXXXXX,            XXXXXXX,
        XXXXXXX,    XXXXXXX,             XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,     XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,            XXXXXXX,  XXXXXXX,
        XXXXXXX,    XXXXXXX,  XXXXXXX,   XXXXXXX,   XXXXXXX,            XXXXXXX,                         XXXXXXX,            XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX),

    [SYB1_L] = LAYOUT_91_ansi(
        XXXXXXX,    XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,     XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        KC_1,       XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,     XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,            XXXXXXX,
        XXXXXXX,    KC_TAB,   XXXXXXX,   XXXXXXX,  XXXXXXX,   KC_EQUAL, MY_COLN,  KC_GRV,      XXXXXXX,  MY_UNDS,  XXXXXXX,  XXXXXXX,  MY_TILD,  MY_PLUS,  XXXXXXX,            XXXXXXX,
        XXXXXXX,    KC_ESC,   KC_BSLS,   KC_MINS,  MY_LABK,   KC_SLSH,  KC_SCLN,  KC_PIPE,     KC_HOME,  KC_COMM,  KC_DOT,   MY_DQUO,  KC_END,             XXXXXXX,            XXXXXXX,
        XXXXXXX,    KC_LSFT,             KC_LBRC,  KC_RBRC,   MY_LPRN,  MY_RPRN,  MY_RABK,     MY_QUES,  MY_LCBR,  MY_RCBR,  XXXXXXX,  KC_QUOT,            XXXXXXX,  XXXXXXX,
        XXXXXXX,    XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,             KC_LCTL,                         KC_RSFT,            XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX)
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode){
        // rshift when holding, 切换到 INPT_L when tap
        case MY_RSFT:{
            //  tap
            if (record->tap.count && record->event.pressed) {
                layer_move(INPT_L);
            }
            else if (record->event.pressed) {
                register_mods(MOD_BIT(KC_RSFT));
            }
            else if (!(record->event.pressed)){
                if(get_mods() & MOD_BIT(KC_RSFT)){
                    unregister_mods(MOD_BIT(KC_RSFT));
                }
            }
            return false;
        }
        // lctrl when holding, 切换到 CTRL_L when tap
        case MY_LCTL:{
            if (record->tap.count && record->event.pressed) {
                layer_move(CTRL_L);
            }
            else if (record->event.pressed) {
                register_mods(MOD_BIT(KC_LCTL));
            }
            else if (!(record->event.pressed)){
                if(get_mods() & MOD_BIT(KC_LCTL)){
                    unregister_mods(MOD_BIT(KC_LCTL));
                }
            }
            return false;
        }
            
        case MY_GOPM:{
            if (record->tap.count && record->event.pressed) {
            }
            else if (record->event.pressed) {
                register_code(MS_DOWN);
            }
            else if (!(record->event.pressed)){
                unregister_code(MS_DOWN);
                tap_code16(MS_BTN1);
                tap_code16(KC_TASK);
            }
            return false;
        }
            
        case MY_CZ:{
            if(record->event.pressed){
                register_code(KC_LCTL);
            }else if(!(record->event.pressed)){
                tap_code(KC_Z);
                unregister_code(KC_LCTL);
            }
            return false;
        }
        case MY_CX:{
            if(record->event.pressed){
                register_code(KC_LCTL);
            }else if(!(record->event.pressed)){
                tap_code(KC_X);
                unregister_code(KC_LCTL);
            }
            return false;
        }
        case MY_CC:{
            if(record->event.pressed){
                register_code(KC_LCTL);
            }else if(!(record->event.pressed)){
                tap_code(KC_C);
                unregister_code(KC_LCTL);
            }
            return false;
        }
        case MY_CV:{
            if(record->event.pressed){
                register_code(KC_LCTL);
            }else if(!(record->event.pressed)){
                tap_code(KC_V);
                unregister_code(KC_LCTL);
            }
            return false;
        }
            
        case MY_BRTH:{
            if(record->event.pressed){
                send_string("030828");
            }
            return false;
        }
            
        case MY_TEST: {
            if(record->event.pressed){
                send_string("@#$^");
            }
            return false;
        }

        case MY_TILD:{
            // ~
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_GRV);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_LPRN:{
            // (
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_9);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_RPRN:{
            // )
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_0);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_UNDS:{
            // _
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_MINS);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_PLUS:{
            // +
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_EQL);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_LCBR:{
            // {
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_LBRC);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_RCBR:{
            // }
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_RBRC);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_PIPE:{
            // |
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_BSLS);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_COLN:{
            // :
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_SCLN);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_DQUO:{
            // "
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_QUOT);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_LABK:{
            // <
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_COMM);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_RABK:{
            // >
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_DOT);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        case MY_QUES:{
            // ?
            if(record->event.pressed){
                register_code(KC_RSFT);
            }else if(!(record->event.pressed)){
                tap_code(KC_SLSH);
                unregister_code(KC_RSFT);
            }
            return false;
        }
        
        
        default:
            return true; // Process all other keycodes normally
    }
    return true;
}

const key_override_t delete_key_override = ko_make_basic(MOD_MASK_SHIFT, KC_BSPC, KC_DEL);
// This globally defines all key overrides to be used
const key_override_t *key_overrides[] = {
	&delete_key_override
};

/*
* tap dance
*/

// Define a type for as many tap dance states as you need
typedef enum {
    TD_NONE
    ,TD_UNKNOWN
    ,TD_SINGLE_TAP
    ,TD_SINGLE_HOLD
    ,TD_DOUBLE_TAP
    ,TD_DOUBLE_HOLD
    ,TD_DOUBLE_SINGLE_TAP // Send two single taps
} td_state_t;

typedef struct {
    bool is_press_action;
    td_state_t state;
} td_tap_t;

// Determine the current tap dance state
td_state_t cur_dance(tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->interrupted || !state->pressed) return TD_SINGLE_TAP;
        // Key has not been interrupted, but the key is still held. Means you want to send a 'HOLD'.
        else return TD_SINGLE_HOLD;
    } else if (state->count == 2) {
        // TD_DOUBLE_SINGLE_TAP is to distinguish between typing "pepper", and actually wanting a double tap
        // action when hitting 'pp'. Suggested use case for this return value is when you want to send two
        // keystrokes of the key, and not the 'double tap' action/macro.
        if (state->interrupted) return TD_DOUBLE_SINGLE_TAP;
        else if (state->pressed) return TD_DOUBLE_HOLD;
        else return TD_DOUBLE_TAP;
    } else return TD_UNKNOWN;
}

/*
* MY_SYBL 
*/
// Initialize tap structure associated with example tap dance key
static td_tap_t sybl_tap_state = {
    .is_press_action = true,
    .state = TD_NONE
};
// Functions that control what our tap dance key does
void sybl_finished(tap_dance_state_t *state, void *user_data) {
    sybl_tap_state.state = cur_dance(state);
    switch (sybl_tap_state.state) {
        case TD_SINGLE_TAP:
            set_oneshot_layer(SYB1_L, ONESHOT_START);
            clear_oneshot_layer_state(ONESHOT_PRESSED);
            break;
        case TD_DOUBLE_TAP:
            layer_move(SYB1_L);
            break;
        default:
            break;
    }
}
void sybl_reset(tap_dance_state_t *state, void *user_data) {
    sybl_tap_state.state = TD_NONE;
}

/* 
* Associate our tap dance key with its functionality
*/
tap_dance_action_t tap_dance_actions[] = {
    [TD_SYBL] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, sybl_finished, sybl_reset)
};

// ENCODER_MAP_ENABLE
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [CTRL_L] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) }
    ,[INPT_L] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) }
    ,[GAME_L] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) }
    ,[CESC_L] = { ENCODER_CCW_CW(RM_VALD, RM_VALU), ENCODER_CCW_CW(RM_VALD, RM_VALU) }
    ,[SYB1_L] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) }
};
#endif 