#pragma once

#include "graphics.h"

typedef struct
{
    // Tiles are 16x16
    TextureEntry *tileset;
    TransformEntry transform;
    int map_w, map_h;
    // 1 instance per tile. instances have just the tile ID
    WGPUBuffer instances;
} Tilemap;

void tilemap_new(Tilemap *tilemap, Graphics *graphics, TextureEntry *tileset,
                 TransformEntry transform, int map_w, int map_h, u16 *map_data);
void tilemap_free(Tilemap *tilemap, Graphics *graphics);

void tilemap_render(Tilemap *tilemap, Graphics *graphics,
                    WGPURenderPassEncoder *pass);
void tilemap_set_tile(Tilemap *tilemap, Graphics *graphics, int x, int y,
                      u16 tile);
