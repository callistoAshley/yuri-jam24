#pragma once
#include "character.h"
#include "graphics/graphics.h"
#include "graphics/sprite.h"

typedef struct
{
    char sprite_name[256];
    char event_name[256];

    Transform transform;
    TransformEntry transform_entry;
    TextureEntry *texture;
    Sprite sprite;
    Quad quad;
    LayerEntry layer_entry;

    Rect rect;
} BasicCharState;

void basic_char_init(void **out, Resources *resources,
                     struct MapScene *map_scene, Rect rect, HashMap *metadata,
                     void *extra_args);

void basic_char_update(void *self, Resources *resources,
                       struct MapScene *map_scene);

void basic_char_free(void *self, Resources *resources,
                     struct MapScene *map_scene);
