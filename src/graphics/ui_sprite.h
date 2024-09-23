#pragma once

#include "graphics.h"

typedef struct
{
    TextureEntry *texture;
    TransformEntry transform;
    QuadEntry quad;

    f32 opacity;
} UiSprite;

void ui_sprite_init(UiSprite *sprite, TextureEntry *texture,
                    TransformEntry transform, QuadEntry quad, f32 opacity);
void ui_sprite_free(UiSprite *sprite, Graphics *graphics);

void ui_sprite_render(UiSprite *sprite, mat4s camera,
                      WGPURenderPassEncoder pass);
