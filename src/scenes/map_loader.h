#pragma once

#include "graphics/layer.h"
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

typedef enum
{
    Layer_Back,
    Layer_Middle,
    Layer_Front,
} MapLayer;

typedef struct
{
    Sprite *sprite;
    Transform transform;
    LayerEntry entry;
} MapSprite;

typedef struct
{
    TilemapLayer *layer;
    LayerEntry entry;
} MapTileLayer;

void handle_map_layers(tmx_layer *head, Resources *resources,
                       b2Vec2 *player_position, MapLoadArgs *map_data);
