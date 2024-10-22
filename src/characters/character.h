#pragma once
#include "scenes/scene.h"
#include "utility/hashmap.h"

struct MapScene;

typedef enum
{
    Char_Basic = 0,
    Char_Autorun,

    Char_Max,
} CharacterType;

typedef void (*character_init_fn)(void **out, Resources *resources,
                                  struct MapScene *map_scene, Rect rect,
                                  HashMap *metadata, void *extra_args);

typedef void (*character_update_fn)(void *self, Resources *resources,
                                    struct MapScene *map_scene);

typedef void (*character_free_fn)(void *self, Resources *resources,
                                  struct MapScene *map_scene);

typedef struct
{
    const char *name;
    CharacterType type;
    character_init_fn init_fn;
    character_update_fn update_fn, fixed_update_fn;
    character_free_fn free_fn;
} CharacterInterface;

extern const CharacterInterface CHARACTERS[Char_Max];
