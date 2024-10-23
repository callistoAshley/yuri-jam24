#pragma once
#include "character.h"
#include "graphics/graphics.h"
#include "graphics/sprite.h"

typedef struct
{
    char sprite_name[256];

    Transform transform;
    Sprite sprite;
    Quad quad;
    LayerEntry layer_entry;

    Rect rect;
    b2BodyId body_id;
    b2Transform old_b2d_position, b2d_position;
} RigidBodyCharState;

void *rigidbody_char_init(Resources *resources, struct MapScene *map_scene,
                          CharacterInitArgs *args);

void rigidbody_char_fixed_update(void *self, Resources *resources,
                                 struct MapScene *map_scene);
void rigidbody_char_update(void *self, Resources *resources,
                           struct MapScene *map_scene);

void rigidbody_char_free(void *self, Resources *resources,
                         struct MapScene *map_scene);
