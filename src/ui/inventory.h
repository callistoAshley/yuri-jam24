#pragma once

#include "fmod_studio_common.h"
#include "graphics/ui_sprite.h"
#include "resources.h"
typedef struct
{
    UiSprite slots[INVENTORY_SIZE];
    LayerEntry slot_entries[INVENTORY_SIZE];

    UiSprite icons[INVENTORY_SIZE];
    LayerEntry icon_entries[INVENTORY_SIZE];
    // a copy of the inventory in Resources. If a pointer in this array
    // mismatches the one in Resources, we update its graphic
    ItemType inventory_view[INVENTORY_SIZE];

    u32 viewed_slot;

    bool open, opening;
    bool closing;

    UiSprite name_sprite;
    LayerEntry name_entry;

    UiSprite desc_sprite;
    LayerEntry desc_entry;

    f32 repeat_timer;

    FMOD_STUDIO_EVENTDESCRIPTION *click_desc;
} Inventory;

void inventory_init(Inventory *inventory, Resources *resources);
void inventory_update(Inventory *inventory, Resources *resources);
void inventory_free(Inventory *inventory, Resources *resources);
