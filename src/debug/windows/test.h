#pragma once
#include "window.h"
#include "utility/macros.h"

extern Window test_window;

void wnd_test_init(Window *self);
void wnd_test_update(Window *self);
void wnd_test_free(Window *self);
