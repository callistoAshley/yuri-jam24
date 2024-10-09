#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include "graphics/wgpu_resources.h"

typedef struct
{
    TTF_Font *font;
} Font;

void font_init(Font *font, const char *path, int size);
void font_free(Font *font);

SDL_Surface *font_render_surface(Font *font, const char *text, SDL_Color color);

void font_texture_size(Font *font, const char *text, int *w, int *h);
WGPUTexture font_render_text(Font *font, const char *text, SDL_Color color,
                             WGPUResources *wgpu);
void font_render_text_to(Font *font, WGPUTexture texture, const char *text,
                         SDL_Color color, WGPUResources *wgpu);
void font_render_text_at(Font *font, u32 x, u32 y, WGPUTexture texture,
                         const char *text, SDL_Color color,
                         WGPUResources *wgpu);
