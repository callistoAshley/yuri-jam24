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

typedef struct
{
    const char *name;
    CharacterType type;
    void (*init_fn)(void **out, Resources *resources,
                    struct MapScene *map_scene, Rect rect, HashMap *metadata,
                    void *extra_args);
    void (*update_fn)(void *self, Resources *resources,
                      struct MapScene *map_scene);
    void (*free_fn)(void *self, Resources *resources,
                    struct MapScene *map_scene);
} CharacterInterface;

extern const CharacterInterface CHARACTERS[Char_Max];
