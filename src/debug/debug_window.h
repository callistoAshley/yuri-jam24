#pragma once
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include "scenes/scene.h" // FIXME: this include is just for the Resources struct,
                          // change it once it's moved

typedef struct
{
    Resources *resources;
    char new_map[256];
} DebugWindowState;

void debug_wnd_show(DebugWindowState *state);
