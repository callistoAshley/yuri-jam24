#pragma once

#include "graphics/tilemap.h"
#include "graphics/light.h"
#include "graphics/sprite.h"
#include "sensible_nums.h"
#include "scene.h"

#include <tmx.h>

typedef struct
{
    i32 *tiles;
    u32 width, height, layers;
    Tilemap *tilemap;
} MapLoadArgs;

void handle_map_layers(tmx_layer *head, Resources *resources,
                       b2Vec2 *player_position, MapLoadArgs *map_data);
