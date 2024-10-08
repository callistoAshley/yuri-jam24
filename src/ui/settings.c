#include "settings.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"

void settings_menu_init(SettingsMenu *menu, Resources *resources)
{
    f32 scale = (f32)resources->graphics->wgpu.surface_config.width /
                INTERNAL_SCREEN_WIDTH;

    {
        Quad quad = {
            .rect = rect_from_size((vec2s){.x = INTERNAL_SCREEN_WIDTH,
                                           .y = INTERNAL_SCREEN_HEIGHT}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics->quad_manager, quad);

        Transform transform = transform_from_scale(VEC3_SPLAT(scale));
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics->transform_manager, transform);

        TextureEntry *background_texture =
            texture_manager_load(&resources->graphics->texture_manager,
                                 TEXTURE_PATH("settings_background.png"),
                                 &resources->graphics->wgpu);

        ui_sprite_init(&menu->background, background_texture, transform_entry,
                       quad_entry, 0.0f);
        menu->bg_entry = layer_add(&resources->graphics->ui_layers.middle,
                                   &menu->background);
    }

    FMOD_Studio_System_GetEvent(resources->audio->system, "event:/menu/hover",
                                &menu->hover_desc);
    FMOD_Studio_System_GetEvent(resources->audio->system, "event:/menu/click",
                                &menu->click_desc);

    const char *categories[] = {
        "Audio",
        "Video",
        "Controls",
        "Back",
    };
    f32 start_y = 60 * scale;
    SDL_Color color = {255, 255, 255, 255};

    for (int i = 0; i < 4; i++)
    {
        const char *category = categories[i];
        WGPUTexture texture =
            font_render_text(&resources->fonts->compaq.medium, category, color,
                             &resources->graphics->wgpu);

        char *path = malloc(strlen(category) + strlen("settings_option") + 1);
        strcpy(path, "settings_option");
        strcat(path, category);
        TextureEntry *texture_entry = texture_manager_register(
            &resources->graphics->texture_manager, texture, path);
        free(path);

        u32 width = wgpuTextureGetWidth(texture);
        u32 height = wgpuTextureGetHeight(texture);

        Transform transform = transform_from_xyz(15, start_y, 0);
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics->transform_manager, transform);
        start_y += height + 5;

        Quad quad = {
            .rect = rect_from_size((vec2s){.x = width, .y = height}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics->quad_manager, quad);

        ui_sprite_init(&menu->categories[i], texture_entry, transform_entry,
                       quad_entry, 0.0f);
        menu->cat_entries[i] = layer_add(
            &resources->graphics->ui_layers.foreground, &menu->categories[i]);
    }

    menu->hovered_category = Cat_None;
    menu->selected_category = Cat_None;

    menu->open = false;
    menu->is_closing = false;
}

void settings_menu_update(SettingsMenu *menu, Resources *resources)
{
    if (!menu->open)
        return;

    bool is_opening = menu->background.opacity < 1.0f && !menu->is_closing;
    if (is_opening)
    {
        menu->background.opacity += 10.0f * resources->input->delta_seconds;
        menu->background.opacity = fminf(menu->background.opacity, 1.0f);

        for (int i = 0; i < 4; i++)
        {
            menu->categories[i].opacity = menu->background.opacity / 2.0;
        }
    }

    if (menu->is_closing)
    {
        menu->background.opacity -= 10.0f * resources->input->delta_seconds;
        menu->background.opacity = fmaxf(menu->background.opacity, 0.0f);

        for (int i = 0; i < 4; i++)
        {
            menu->categories[i].opacity = menu->background.opacity / 2.0;
        }

        if (menu->background.opacity == 0.0f)
        {
            menu->open = false;
            menu->is_closing = false;

            menu->selected_category = Cat_None;
            menu->hovered_category = Cat_None;

            return;
        }
    }

    f32 scale = (f32)resources->graphics->wgpu.surface_config.width /
                INTERNAL_SCREEN_WIDTH;

    if (resources->graphics->was_resized)
    {
        Transform transform = transform_from_scale(VEC3_SPLAT(scale));
        transform_manager_update(&resources->graphics->transform_manager,
                                 menu->background.transform, transform);
    }

    bool did_hover_option = false;
    bool hovered_new_option = false;

    f32 mouse_x = resources->input->mouse_x;
    f32 mouse_y = resources->input->mouse_y;
    f32 start_y = 60 * scale;

    for (SettingsCategory i = 0; i < 4; i++)
    {
        UiSprite *category = &menu->categories[i];

        WGPUTexture texture = texture_manager_get_texture(
            &resources->graphics->texture_manager, category->texture);
        u32 width = wgpuTextureGetWidth(texture);
        u32 height = wgpuTextureGetHeight(texture);

        f32 sprite_y = start_y;
        start_y += height + 5;

        // uh oh, looks like we've resized. we need to reposition the options
        if (resources->graphics->was_resized)
        {
            Transform transform = transform_from_xyz(15, sprite_y, 0);
            transform_manager_update(&resources->graphics->transform_manager,
                                     category->transform, transform);
        }

        if (menu->is_closing || is_opening)
            break;

        bool outside_y = mouse_y < sprite_y || mouse_y > sprite_y + height;
        bool outside_x = mouse_x < 15 || mouse_x > 15 + width;
        if (outside_x || outside_y)
        {
            category->opacity = menu->background.opacity / 2.0;
        }
        else
        {
            category->opacity = menu->background.opacity;
            did_hover_option = true;
            hovered_new_option = i != menu->hovered_category;
            menu->hovered_category = i;
        }

        if (i == menu->selected_category)
        {
            category->opacity = 1.0f;
        }
    }

    if (!did_hover_option)
    {
        menu->hovered_category = Cat_None;
    }

    // fire and forget event instances
    // FMOD will automatically release the instance when it's finished
    if (hovered_new_option)
    {
        FMOD_STUDIO_EVENTINSTANCE *instance;
        FMOD_Studio_EventDescription_CreateInstance(menu->hover_desc,
                                                    &instance);

        FMOD_Studio_EventInstance_Start(instance);
        FMOD_Studio_EventInstance_Release(instance);
    }

    if (input_is_pressed(resources->input, Button_Back) && !is_opening)
    {
        menu->is_closing = true;
    }

    bool did_select_option =
        menu->hovered_category != Cat_None &&
        menu->selected_category != menu->hovered_category &&
        input_is_pressed(resources->input, Button_MouseLeft);
    if (did_select_option)
    {
        menu->selected_category = menu->hovered_category;

        FMOD_STUDIO_EVENTINSTANCE *instance;
        FMOD_Studio_EventDescription_CreateInstance(menu->click_desc,
                                                    &instance);

        FMOD_Studio_EventInstance_Start(instance);
        FMOD_Studio_EventInstance_Release(instance);

        switch (menu->selected_category)
        {
        case Cat_Back:
            menu->is_closing = true;
            break;
        default:
            break;
        }
    }
}

void settings_menu_free(SettingsMenu *menu, Resources *resources)
{
    ui_sprite_free(&menu->background, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.middle, menu->bg_entry);

    for (int i = 0; i < 4; i++)
    {
        ui_sprite_free(&menu->categories[i], resources->graphics);
        layer_remove(&resources->graphics->ui_layers.foreground,
                     menu->cat_entries[i]);
    }
}
