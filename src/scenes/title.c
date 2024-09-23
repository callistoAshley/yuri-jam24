#include "title.h"
#include "fonts/font.h"
#include "graphics/tex_manager.h"
#include "input/input.h"
#include "scenes/map.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"
#include <string.h>

void title_scene_init(Scene **scene_data, Resources *resources)
{
    TitleScene *title_scene = malloc(sizeof(TitleScene));
    title_scene->type = Scene_Title;
    *scene_data = (Scene *)title_scene;

    title_scene->selected_option = -1;

    FMOD_STUDIO_EVENTDESCRIPTION *desc;
    FMOD_RESULT result;
    result = FMOD_Studio_System_GetEvent(resources->audio->system,
                                         "event:/godscape", &desc);
    FMOD_ERRCHK(result, "Failed to get event");
    FMOD_Studio_EventDescription_CreateInstance(desc, &title_scene->bgm);
    FMOD_Studio_EventInstance_Start(title_scene->bgm);

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

        TextureEntry *background_texture = texture_manager_load(
            &resources->graphics->texture_manager,
            TEXTURE_PATH("titlescreen.png"), &resources->graphics->wgpu);

        ui_sprite_init(&title_scene->background, background_texture,
                       transform_entry, quad_entry, 1.0f);
        title_scene->background_entry =
            layer_add(&resources->graphics->ui_layers.background,
                      &title_scene->background);
    }

    Font font;
    font_init(&font, "assets/fonts/Mx437_Compaq_Port3.ttf", 32);

    const char *options[] = {
        "Start",
        "Settings",
        "Quit",
    };
    SDL_Color color = {255, 255, 255, 255};

    f32 start_y = 70 * scale;
    for (u32 i = 0; i < 3; i++)
    {
        WGPUTexture texture = font_render_text(&font, options[i], color,
                                               &resources->graphics->wgpu);
        u32 width = wgpuTextureGetWidth(texture);
        u32 height = wgpuTextureGetHeight(texture);

        Quad quad = {
            .rect = rect_from_size((vec2s){.x = width, .y = height}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };

        f32 x = (INTERNAL_SCREEN_WIDTH * scale - width) / 2;
        Transform transform = transform_from_xyz(x, start_y, 0);
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics->transform_manager, transform);
        start_y += height + 5;

        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics->quad_manager, quad);

        char *path = malloc(strlen(options[i]) + strlen("ttl_option") + 1);
        strcpy(path, "ttl_option");
        strcat(path, options[i]);
        TextureEntry *texture_entry = texture_manager_register(
            &resources->graphics->texture_manager, texture, path);
        free(path);

        ui_sprite_init(&title_scene->options[i], texture_entry, transform_entry,
                       quad_entry, 0.5f);
        title_scene->option_entries[i] = layer_add(
            &resources->graphics->ui_layers.middle, &title_scene->options[i]);
    }

    font_free(&font);
}

void title_scene_update(Scene *scene_data, Resources *resources)
{
    TitleScene *title_scene = (TitleScene *)scene_data;
    (void)resources;

    f32 mouse_x = resources->input->mouse_x;
    f32 mouse_y = resources->input->mouse_y;

    f32 scale = (f32)resources->graphics->wgpu.surface_config.width /
                INTERNAL_SCREEN_WIDTH;
    f32 start_y = 70 * scale;

    if (resources->graphics->was_resized)
    {
        Transform transform = transform_from_scale(VEC3_SPLAT(scale));
        transform_manager_update(&resources->graphics->transform_manager,
                                 title_scene->background.transform, transform);
    }

    bool selected_option = false;
    for (u32 i = 0; i < 3; i++)
    {
        UiSprite *option = &title_scene->options[i];

        WGPUTexture texture = texture_manager_get_texture(
            &resources->graphics->texture_manager, option->texture);
        u32 width = wgpuTextureGetWidth(texture);
        u32 height = wgpuTextureGetHeight(texture);

        f32 sprite_x = (INTERNAL_SCREEN_WIDTH * scale - width) / 2;

        // uh oh, looks like we've resized. we need to reposition the options
        if (resources->graphics->was_resized)
        {
            Transform transform = transform_from_xyz(sprite_x, start_y, 0);
            transform_manager_update(&resources->graphics->transform_manager,
                                     option->transform, transform);
        }

        bool outside_y = mouse_y < start_y || mouse_y > start_y + height;
        bool outside_x = mouse_x < sprite_x || mouse_x > sprite_x + width;
        if (outside_x || outside_y)
        {
            option->opacity = 0.5f;
        }
        else
        {
            option->opacity = 1.0f;
            selected_option = true;
            title_scene->selected_option = i;
        }

        start_y += height + 5;
    }

    if (!selected_option)
        title_scene->selected_option = -1;

    if (input_is_pressed(resources->input, Button_MouseLeft))
    {
        switch (title_scene->selected_option)
        {
        case 0:
            scene_change(MAP_SCENE, resources);
            break;
        case 2:
            resources->input->requested_quit = true;
            break;
        default:
            break;
        }
    }
}

void title_scene_free(Scene *scene_data, Resources *resources)
{
    TitleScene *title_scene = (TitleScene *)scene_data;
    FMOD_Studio_EventInstance_Stop(title_scene->bgm,
                                   FMOD_STUDIO_STOP_IMMEDIATE);
    FMOD_Studio_EventInstance_Release(title_scene->bgm);

    ui_sprite_free(&title_scene->background, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.background,
                 title_scene->background_entry);

    for (u32 i = 0; i < 3; i++)
    {
        ui_sprite_free(&title_scene->options[i], resources->graphics);
        layer_remove(&resources->graphics->ui_layers.middle,
                     title_scene->option_entries[i]);
    }

    free(title_scene);
}

const SceneInterface TITLE_SCENE = {
    .init = title_scene_init,
    .update = title_scene_update,
    .free = title_scene_free,
};
