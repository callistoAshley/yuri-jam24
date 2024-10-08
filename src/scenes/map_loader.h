#pragma once

#include "graphics/caster_manager.h"
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

    b2Vec2 *player_position;

    vec *colliders;
    vec *renderables;
} MapLoadArgs;

typedef enum
{
    Layer_Back,
    Layer_Middle,
    Layer_Front,
} MapLayer;

typedef struct
{
    enum
    {
        Map_Sprite,
        Map_TileLayer,
        Map_Caster,
        Map_Light,
    } type;

    // we could make these not be pointers if we filled in layers later (and
    // didn't care about removing/adding things...)
    union
    {
        struct
        {
            Sprite *ptr;
            MapLayer layer;
        } sprite;
        struct
        {
            TilemapLayer *ptr;
            MapLayer layer;
        } tile;
        ShadowCaster *caster;
        Light *light;
    } data;

    LayerEntry entry;
} MapRenderable;

void handle_map_layers(tmx_layer *head, Resources *resources,
                       MapLoadArgs *map_data);
