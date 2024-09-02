#pragma once
#include <string.h>

#include "window.h"
#include "utility/log.h"
#include "utility/macros.h"

typedef struct
{
    char input_name[256];
    int input_width, input_height;
} NewMapInfo;

typedef void (*NewMapDoneFn)(void *wnd_cont, NewMapInfo info);

extern Window new_map_window;

void wnd_new_map_init(Window *self);
void wnd_new_map_set_done_callback(Window *self, NewMapDoneFn fn);
void wnd_new_map_update(Window *self);
void wnd_new_map_free(Window *self);
