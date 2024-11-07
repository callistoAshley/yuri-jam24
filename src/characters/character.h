#pragma once
#include "tmx.h"
#include "utility/hashmap.h"
#include "resources.h"

struct MapScene;

typedef enum
{
    Char_Basic = 0,
    Char_Autorun,
    Char_RigidBody,

    Char_Max,
} CharacterType;

// we pass arguments as a struct so adding new fields to said struct won't
// disrupt all the existing init functions
typedef struct
{
    Rect rect;
    f32 rotation;
    HashMap *metadata;
    void *extra_args;
    enum tmx_obj_type object_type;
} CharacterInitArgs;

typedef void *(*character_init_fn)(Resources *resources,
                                   struct MapScene *map_scene,
                                   CharacterInitArgs *args);

// characters can update the self pointer if they so choose
// (this is used by the autorun character to use less resources)
typedef void (*character_update_fn)(void **self, Resources *resources,
                                    struct MapScene *map_scene);

typedef void (*character_free_fn)(void *self, Resources *resources,
                                  struct MapScene *map_scene);

typedef struct
{
    const char *name;
    character_init_fn init_fn;
    character_update_fn update_fn, fixed_update_fn;
    character_free_fn free_fn;
} CharacterInterface;

extern const CharacterInterface CHARACTERS[Char_Max];
