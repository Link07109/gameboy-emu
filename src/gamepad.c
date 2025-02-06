#include "../include/gamepad.h"

typedef struct {
    bool button_selected;
    bool direction_selected;
    gamepad_state controller;
} gamepad_context;

static gamepad_context gamepad_ctx = { 0 };

gamepad_state* gamepad_get_state() {
    return &gamepad_ctx.controller;
}

void gamepad_init() {
}

void gamepad_set_selected(u8 value) {
    gamepad_ctx.button_selected = value & 0x20;
    gamepad_ctx.direction_selected = value & 0x10;
}

bool gamepad_button_selected() {
    return gamepad_ctx.button_selected;
}

bool gamepad_direction_selected() {
    return gamepad_ctx.direction_selected;
}

u8 gamepad_get_output() {
    u8 output = 0xCF;

    if (!gamepad_button_selected()) {
        if (gamepad_get_state()->start) {
            output &= ~(1 << 3);
        }
        if (gamepad_get_state()->select) {
            output &= ~(1 << 2);
        }
        if (gamepad_get_state()->b) {
            output &= ~(1 << 1);
        }
        if (gamepad_get_state()->a) {
            output &= ~(1 << 0);
        }
    }

    if (!gamepad_direction_selected()) {
        if (gamepad_get_state()->down) {
            output &= ~(1 << 3);
        }
        if (gamepad_get_state()->up) {
            output &= ~(1 << 2);
        }
        if (gamepad_get_state()->left) {
            output &= ~(1 << 1);
        }
        if (gamepad_get_state()->right) {
            output &= ~(1 << 0);
        }
    }

    return output;
}
