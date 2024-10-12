#pragma once

#include "graphics.h"

typedef struct
{
    TextureEntry *texture;
    TransformEntry transform;
    QuadEntry quad;

    vec2s parallax_factor;
} Sprite;

void sprite_init(Sprite *sprite, TextureEntry *texture,
                 TransformEntry transform, QuadEntry quad);
void sprite_free(Sprite *sprite, Graphics *graphics);

void sprite_render(Sprite *sprite, mat4s camera, WGPURenderPassEncoder pass);
