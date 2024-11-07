#pragma once
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include "resources.h"

typedef struct
{
    Resources *resources;
    char new_map[256];
} DebugWindowState;

void debug_wnd_show(DebugWindowState *state);
