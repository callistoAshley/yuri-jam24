#pragma once
#include "scenes/scene.h"
#include "utility/hashmap.h"

struct MapScene;

typedef struct
{
    const char *name;
    void (*init_fn)(void **out, Resources *resources, struct MapScene *map_scene, Rect rect, HashMap *metadata, void *extra_args);
    void (*update_fn)(void *self, Resources *resources, struct MapScene *map_scene);
    void (*free_fn)(void *self, Resources *resources, struct MapScene *map_scene);
} CharacterInterface;
