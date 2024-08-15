#pragma once
#include "window.h"
#include "utility/macros.h"

extern Window tmap_edit_window;

void wnd_tmap_edit_init(Window *self);
void wnd_tmap_edit_update(Window *self);
void wnd_tmap_edit_free(Window *self);
