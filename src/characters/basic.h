#pragma once
#include "character.h"
#include "events/vm.h"
#include "graphics/graphics.h"
#include "graphics/sprite.h"

typedef struct
{
    char event_name[256];
    // VMs take a lot of space (~2kb) so it's better to eat the heap
    // allocation when running an event than it is to store a VM 24/7
    VM *vm;

    Transform transform;
    Quad quad;

    Sprite sprite;
    LayerEntry layer_entry;

    ShadowCaster caster;
    LayerEntry caster_entry;

    Rect rect;
} BasicCharState;

void *basic_char_init(Resources *resources, struct MapScene *map_scene,
                      CharacterInitArgs *args);

void basic_char_update(void **self, Resources *resources,
                       struct MapScene *map_scene);

void basic_char_fixed_update(void **self, Resources *resources,
                             struct MapScene *map_scene);

void basic_char_free(void *self, Resources *resources,
                     struct MapScene *map_scene);
