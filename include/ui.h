#pragma once

#include "common.h"

static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 720;

static const int DEBUG_SCREEN_WIDTH = 510;
static const int DEBUG_SCREEN_HEIGHT = 798;

void ui_init();
void ui_handle_events();
void ui_update();
