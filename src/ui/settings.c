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
        menu->bg_entry = layer_add(&resources->graphics->ui_layers.foreground,
                                   &menu->background);
    }

    menu->open = false;
}

void settings_menu_update(SettingsMenu *menu, Resources *resources)
{
    if (!menu->open)
        return;

    bool is_opening = menu->background.opacity < 1.0f;
    if (is_opening)
    {
        menu->background.opacity += 10.0f * resources->input->delta_seconds;
        menu->background.opacity = fminf(menu->background.opacity, 1.0f);
    }
}

void settings_menu_free(SettingsMenu *menu, Resources *resources)
{
    ui_sprite_free(&menu->background, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.foreground, menu->bg_entry);
}
