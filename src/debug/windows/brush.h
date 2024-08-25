#pragma once
#include "window.h"
#include "graphics/graphics.h"
#include "graphics/tex_manager.h"
#include "utility/macros.h"

extern Window brush_window;

void wnd_brush_init(Window *self);
void wnd_brush_set_tileset(Window *self, char *tileset);
void wnd_brush_init_tilemap(
    Window *self, 
    uint32_t width, 
    uint32_t height);
void wnd_brush_update(Window *self);
void wnd_brush_free(Window *self);
