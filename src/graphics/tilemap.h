#pragma once

#include "graphics.h"

typedef struct
{
    // Tiles are 8x8
    TextureEntry *tileset;
    TransformEntry transform;
    int map_w, map_h, layers;
    // 1 instance per tile. instances have just the tile ID
    WGPUBuffer instances;
} Tilemap;

typedef struct
{
    Tilemap *tilemap;
    int layer;

    vec2s parallax_factor;
} TilemapLayer;

void tilemap_init(Tilemap *tilemap, Graphics *graphics, TextureEntry *tileset,
                  TransformEntry transform, int map_w, int map_h, int layers,
                  i32 *map_data);
void tilemap_free(Tilemap *tilemap, Graphics *graphics);

void tilemap_render(Tilemap *tilemap, mat4s camera, int layer,
                    WGPURenderPassEncoder pass);
