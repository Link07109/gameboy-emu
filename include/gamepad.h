#pragma once

#include "common.h"

typedef struct {
    bool start;
    bool select;
    bool a;
    bool b;
    bool up;
    bool down;
    bool left;
    bool right;
} gamepad_state;

gamepad_state* gamepad_get_state();

void gamepad_init();
bool gamepad_button_selected();
bool gamepad_direction_selected();
void gamepad_set_selected(u8 value);

u8 gamepad_get_output();
