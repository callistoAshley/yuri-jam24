#pragma once
#include "character.h"
#include "graphics/graphics.h"
#include "graphics/sprite.h"

typedef struct
{
    char sprite_name[256];
    char event_name[256];

    Transform transform;
    Quad quad;
    Sprite sprite;
    LayerEntry layer_entry;

    Rect rect;
} BasicCharState;

void *basic_char_init(Resources *resources, struct MapScene *map_scene,
                      CharacterInitArgs *args);

void basic_char_update(void *self, Resources *resources,
                       struct MapScene *map_scene);

void basic_char_free(void *self, Resources *resources,
                     struct MapScene *map_scene);
