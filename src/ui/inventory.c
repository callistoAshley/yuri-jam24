#include "inventory.h"
#include "core_types.h"
#include "fmod_studio.h"
#include "fmod_studio_common.h"
#include "graphics/tex_manager.h"
#include "items/item.h"
#include "resources.h"
#include "utility/common_defines.h"

void inventory_init(Inventory *inventory, Resources *resources)
{
    inventory->viewed_slot = 0;

    inventory->open = false;
    inventory->closing = false;

    inventory->repeat_timer = 0.0;

    FMOD_Studio_System_GetEvent(resources->audio.system,
                                "event:/menu/inventory-click",
                                &inventory->click_desc);

    memcpy(inventory->inventory_view, resources->inventory,
           sizeof(resources->inventory));

    for (u32 i = 0; i < INVENTORY_SIZE; i++)
    {
        // while we *could* deduplicate the quad, other infrastructure is not
        // setup to handle that (also we don't need to anyway...)
        Quad quad = {
            .rect = rect_from_size(VEC2_SPLAT(32.0)),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        Transform transform = TRANSFORM_UNIT;

        // initialize slot
        {
            TextureEntry *texture = texture_manager_load(
                &resources->graphics.texture_manager,
                "assets/textures/slot.png", &resources->graphics.wgpu);

            QuadEntry quad_entry =
                quad_manager_add(&resources->graphics.quad_manager, quad);

            TransformEntry transform_entry = transform_manager_add(
                &resources->graphics.transform_manager, transform);

            ui_sprite_init(&inventory->slots[i], texture, transform_entry,
                           quad_entry, 0.0);
            inventory->slot_entries[i] = layer_add(
                &resources->graphics.ui_layers.middle, &inventory->slots[i]);
        }

        // initialize slot graphic
        ItemType item_type = inventory->inventory_view[i];
        if (item_type != Item_None)
        {
            Item item = ITEMS[item_type];

            TextureEntry *texture =
                texture_manager_load(&resources->graphics.texture_manager,
                                     item.icon, &resources->graphics.wgpu);

            QuadEntry quad_entry =
                quad_manager_add(&resources->graphics.quad_manager, quad);

            TransformEntry transform_entry = transform_manager_add(
                &resources->graphics.transform_manager, transform);

            ui_sprite_init(&inventory->icons[i], texture, transform_entry,
                           quad_entry, 0.0);
            inventory->icon_entries[i] =
                layer_add(&resources->graphics.ui_layers.foreground,
                          &inventory->icons[i]);
        }
    }
}

void inventory_update(Inventory *inventory, Resources *resources)
{
    if (!inventory->open)
        return;

    f32 delta = time_delta_seconds(resources->time.current);

    for (u32 i = 0; i < INVENTORY_SIZE; i++)
    {
        ItemType item_type = inventory->inventory_view[i];
        ItemType actual_item_type = resources->inventory[i];

        // if the displayed item doesn't match what the item actually is, update
        // things accordingly
        if (item_type != actual_item_type)
        {
            inventory->inventory_view[i] = item_type;
            item_type = actual_item_type;
            // this branch can only be reached if this slot used to have an
            // item, so we need to release it
            if (item_type == Item_None)
            {
                ui_sprite_free(&inventory->icons[i], &resources->graphics);
                layer_remove(&resources->graphics.ui_layers.foreground,
                             inventory->icon_entries[i]);
            }
            else
            {
                // it's a different item, so we unload the current
                // icon texture and swap in a new one
                texture_manager_unload(&resources->graphics.texture_manager,
                                       inventory->icons[i].texture);

                Item item = ITEMS[item_type];
                TextureEntry *texture =
                    texture_manager_load(&resources->graphics.texture_manager,
                                         item.icon, &resources->graphics.wgpu);
                inventory->icons[i].texture = texture;
            }
        }

        bool is_viewed = inventory->viewed_slot == i;

        f32 max_opacity = 0.25;
        if (is_viewed)
            max_opacity = 1.0;

        f32 opacity = inventory->slots[i].opacity;
        if (inventory->opening)
        {
            opacity += 5.0 * delta;
            opacity = fmin(opacity, max_opacity);
        }
        else if (inventory->closing)
        {
            opacity -= 5.0 * delta;
            opacity = fmax(opacity, 0.0);
        }
        else
        {
            opacity = max_opacity;
        }

        inventory->slots[i].opacity = opacity;
        if (item_type != Item_None)
            inventory->icons[i].opacity = opacity;

        f32 center_x = UI_VIEW_WIDTH / 2.0;
        i32 centered_i = (i32)i - inventory->viewed_slot;
        vec3s position = {
            .x = center_x + (centered_i * 48.0 - 16.0) * UI_SCALE,
            .y = UI_VIEW_HEIGHT - 48.0 * UI_SCALE,
        };
        Transform transform =
            transform_from_position_scale(position, VEC3_SPLAT(UI_SCALE));
        transform_manager_update(&resources->graphics.transform_manager,
                                 inventory->slots[i].transform, transform);
        if (item_type != Item_None)
        {
            transform_manager_update(&resources->graphics.transform_manager,
                                     inventory->icons[i].transform, transform);
        }
    }

    f32 viewed_opacity = inventory->slots[inventory->viewed_slot].opacity;
    if (inventory->opening && viewed_opacity >= 1.0)
    {
        inventory->opening = false;
    }

    if (inventory->closing && viewed_opacity <= 0.0)
    {
        inventory->closing = false;
        inventory->open = false;
        return;
    }

    if (inventory->opening || inventory->closing)
        return;

    if (input_did_press(&resources->input, Button_Back))
    {
        inventory->closing = true;
        return;
    }

    bool left_clicked = input_did_press(&resources->input, Button_Left);
    bool right_clicked = input_did_press(&resources->input, Button_Right);

    bool left_down = input_is_down(&resources->input, Button_Left);
    bool right_down = input_is_down(&resources->input, Button_Right);

    if (left_clicked || right_clicked)
        inventory->repeat_timer = 0.5;

    if (left_down || right_down)
        inventory->repeat_timer -= delta;

    bool repeat = false;
    if (inventory->repeat_timer <= 0.0)
    {
        inventory->repeat_timer = 0.07;
        repeat = true;
    }

    bool left_input = left_clicked || (left_down && repeat);
    bool right_input = right_clicked || (right_down && repeat);

    bool changed_slot = false;
    if (left_input && inventory->viewed_slot > 0)
    {
        inventory->viewed_slot--;
        changed_slot = true;
    }
    else if (right_input && inventory->viewed_slot < INVENTORY_SIZE - 1)
    {
        inventory->viewed_slot++;
        changed_slot = true;
    }

    if (changed_slot)
    {
        FMOD_STUDIO_EVENTINSTANCE *inst;
        FMOD_Studio_EventDescription_CreateInstance(inventory->click_desc,
                                                    &inst);
        FMOD_Studio_EventInstance_Start(inst);
        FMOD_Studio_EventInstance_Release(inst);
    }
}

void inventory_free(Inventory *inventory, Resources *resources)
{
    for (u32 i = 0; i < INVENTORY_SIZE; i++)
    {
        ui_sprite_free(&inventory->slots[i], &resources->graphics);
        layer_remove(&resources->graphics.ui_layers.middle,
                     inventory->slot_entries[i]);

        // free slot graphic (if it has one)
        ItemType item_type = inventory->inventory_view[i];
        if (item_type != Item_None)
        {
            ui_sprite_free(&inventory->icons[i], &resources->graphics);
            layer_remove(&resources->graphics.ui_layers.foreground,
                         inventory->icon_entries[i]);
        }
    }
}
