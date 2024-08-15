#pragma once
#include <string.h>

#include "window.h"
#include "utility/macros.h"

extern Window new_map_window;

void wnd_new_map_init(Window *self);
void wnd_new_map_update(Window *self);
void wnd_new_map_free(Window *self);
