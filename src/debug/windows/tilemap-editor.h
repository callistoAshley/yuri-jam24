#pragma once
#include "window.h"
#include "graphics/tilemap.h"
#include "graphics/transform_manager.h"
#include "utility/macros.h"
#include "core_types.h"

extern Window tmap_edit_window;

void wnd_tmap_edit_init(Window *self);
void wnd_tmap_edit_init_tilemap(
    Window *self, 
    uint32_t width, 
    uint32_t height,
    TextureEntry *tileset_tex,
    WGPUTextureView tileset_tex_non_srgb);
void wnd_tmap_edit_update(Window *self);
void wnd_tmap_edit_free(Window *self);
