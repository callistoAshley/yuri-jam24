#include "settings.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_surface.h"
#include "fmod_studio_common.h"
#include "fonts/font.h"
#include "graphics/tex_manager.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"
#include "utility/graphics.h"
#include "webgpu.h"

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

    u32 max_width = 0;
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

        if (width > max_width)
            max_width = width;

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

    {
        WGPUTexture texture = blank_texture(400, 200,
                                            WGPUTextureUsage_CopyDst |
                                                WGPUTextureUsage_TextureBinding,
                                            &resources->graphics->wgpu);
        TextureEntry *texture_entry =
            texture_manager_register(&resources->graphics->texture_manager,
                                     texture, "settings_menu_category");

        Transform transform =
            transform_from_xyz(15 + max_width + 15, 60 * scale, 0);
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics->transform_manager, transform);

        Quad quad = {
            .rect = rect_from_size((vec2s){.x = 400, .y = 200}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics->quad_manager, quad);

        ui_sprite_init(&menu->category, texture_entry, transform_entry,
                       quad_entry, 0.0f);
        menu->category_entry = layer_add(
            &resources->graphics->ui_layers.foreground, &menu->category);
    }
    menu->category_surf = SDL_CreateSurface(400, 200, SDL_PIXELFORMAT_RGBA32);

    menu->hovered_category = Cat_None;
    menu->selected_category = Cat_None;

    menu->open = false;
    menu->is_closing = false;

    menu->repeat_input_timer = 0.05;
}

static void fire_and_forget(FMOD_STUDIO_EVENTDESCRIPTION *desc)
{
    FMOD_STUDIO_EVENTINSTANCE *instance;
    FMOD_Studio_EventDescription_CreateInstance(desc, &instance);

    FMOD_Studio_EventInstance_Start(instance);
    FMOD_Studio_EventInstance_Release(instance);
}

static void draw_number_selector_to(SDL_Surface *surface, Font *font, i32 x,
                                    i32 y, u32 value)
{
    char text[20];
    snprintf(text, 20, "< %3d >", value);

    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface *src = font_render_surface(font, text, color);
    SDL_Rect dst_rect = {.x = x, .y = y, .w = src->w, .h = src->h};
    SDL_BlitSurface(src, NULL, surface, &dst_rect);
    SDL_DestroySurface(src);
}

static void draw_text_selector_to(SDL_Surface *surface, Font *font, i32 x,
                                  i32 y, const char *value, u32 max_len)
{
    u32 len = strlen(value);
    u32 left_pad = (max_len - len) / 2;
    u32 right_pad = left_pad;
    if (len % 2 == 1)
        right_pad++;

    char text[20];
    snprintf(text, 20, "< %*s%s%*s >", left_pad, "", value, right_pad, "");

    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface *src = font_render_surface(font, text, color);
    SDL_Rect dst_rect = {.x = x, .y = y, .w = src->w, .h = src->h};
    SDL_BlitSurface(src, NULL, surface, &dst_rect);
    SDL_DestroySurface(src);
}

static void draw_text_at(SDL_Surface *surface, Font *font, i32 x, i32 y,
                         const char *text)
{
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface *src = font_render_surface(font, text, color);
    SDL_Rect dst_rect = {.x = x, .y = y, .w = src->w, .h = src->h};
    SDL_BlitSurface(src, NULL, surface, &dst_rect);
    SDL_DestroySurface(src);
}

static char *text_of_vsync_mode(WGPUPresentMode mode)
{
    switch (mode)
    {

    case WGPUPresentMode_Fifo:
        return "Fifo";
    case WGPUPresentMode_FifoRelaxed:
        return "Relaxed";
    case WGPUPresentMode_Immediate:
        return "Immediate";
    case WGPUPresentMode_Mailbox:
        return "Mailbox";
    // this should NEVER happen
    case WGPUPresentMode_Force32:
        return "oops";
    }
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

        menu->category.opacity = 0.0;

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
    u32 max_width = 0;

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

        if (width > max_width)
            max_width = width;

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

    if (resources->graphics->was_resized)
    {
        Transform transform =
            transform_from_xyz(15 + max_width + 15, 60 * scale, 0);
        transform_manager_update(&resources->graphics->transform_manager,
                                 menu->category.transform, transform);
    }

    if (!did_hover_option)
    {
        menu->hovered_category = Cat_None;
    }

    // fire and forget event instances
    // FMOD will automatically release the instance when it's finished
    if (hovered_new_option)
        fire_and_forget(menu->hover_desc);

    if (input_is_pressed(resources->input, Button_Back) && !is_opening)
    {
        menu->is_closing = true;
    }

    bool mouse_clicked = input_is_pressed(resources->input, Button_MouseLeft);

    bool did_select_option =
        menu->hovered_category != Cat_None &&
        menu->selected_category != menu->hovered_category && mouse_clicked;
    if (did_select_option)
    {
        menu->selected_category = menu->hovered_category;

        fire_and_forget(menu->click_desc);

        switch (menu->selected_category)
        {
        case Cat_Back:
            menu->is_closing = true;
            break;
        default:
            menu->category.opacity = 1.0;
            break;
        }
    }

    Font *category_font = &resources->fonts->compaq.medium;

    i32 character_width, character_height;
    font_texture_size(category_font, " ", &character_width, &character_height);

    i32 relative_mouse_x = mouse_x - (15 + max_width + 15);
    i32 relative_mouse_y = mouse_y - (60 * scale);

    bool mouse_down = input_is_down(resources->input, Button_MouseLeft);

    // TODO: add repeat logic to input
    if (mouse_clicked)
        menu->repeat_input_timer = 0.5;

    if (mouse_down)
        menu->repeat_input_timer -= resources->input->delta_seconds;

    bool repeat = false;
    if (menu->repeat_input_timer <= 0)
    {
        menu->repeat_input_timer = 0.05;
        repeat = true;
    }

    bool repeat_clicked = (mouse_down && repeat) || mouse_clicked;

#define MOUSE_INSIDE_BUTTON(x, y)                                              \
    (relative_mouse_x >= x && relative_mouse_x <= x + character_width &&       \
     relative_mouse_y >= y && relative_mouse_y <= y + character_height)

    SDL_ClearSurface(menu->category_surf, 0, 0, 0, 0);
    switch (menu->selected_category)
    {
    case Cat_Audio:
    {
        // bgm volume
        draw_text_at(menu->category_surf, category_font, 0, 0, "BGM Volume");
        // sfx volume
        draw_text_at(menu->category_surf, category_font, 0,
                     character_height + 5, "SFX Volume");
        // bgm volume number
        draw_number_selector_to(menu->category_surf, category_font, 180, 2,
                                resources->settings->audio.bgm_volume);
        // sfx volume number
        draw_number_selector_to(menu->category_surf, category_font, 180,
                                2 + character_height + 5,
                                resources->settings->audio.sfx_volume);

        i32 button_x = 180;
        i32 button_y = 2;

        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && repeat_clicked)
        {

            if (resources->settings->audio.bgm_volume > 0)
            {
                resources->settings->audio.bgm_volume--;
                fire_and_forget(menu->hover_desc);
            }
        }

        button_x = button_x + (character_width * 6);
        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && repeat_clicked)
        {
            if (resources->settings->audio.bgm_volume < 100)
            {
                resources->settings->audio.bgm_volume++;
                fire_and_forget(menu->hover_desc);
            }
        }

        button_x = 180;
        button_y = character_height + 5 + 2;
        if (MOUSE_INSIDE_BUTTON(button_x, button_y))
        {
            if (repeat_clicked)
            {
                if (resources->settings->audio.sfx_volume > 0)
                {
                    resources->settings->audio.sfx_volume--;
                    fire_and_forget(menu->hover_desc);
                }
            }
        }

        button_x = button_x + (character_width * 6);
        if (MOUSE_INSIDE_BUTTON(button_x, button_y))
        {
            if (repeat_clicked)
            {
                if (resources->settings->audio.sfx_volume < 100)
                {
                    resources->settings->audio.sfx_volume++;
                    fire_and_forget(menu->hover_desc);
                }
            }
        }

        WGPUTexture texture = texture_manager_get_texture(
            &resources->graphics->texture_manager, menu->category.texture);
        write_surface_to_texture(menu->category_surf, texture,
                                 &resources->graphics->wgpu);
        break;
    }
    case Cat_Video:
    {

        // vsync
        draw_text_at(menu->category_surf, category_font, 0, 0, "VSYNC Mode");

        draw_text_at(menu->category_surf, category_font, 0,
                     character_height + 5, "Fullscreen");

        WGPUPresentMode current_mode = resources->settings->video.present_mode;
        if (current_mode == WGPUPresentMode_Immediate ||
            current_mode == WGPUPresentMode_Mailbox)
        {
            draw_text_at(menu->category_surf, category_font, 0,
                         (character_height + 5) * 2, "Max Framerate");
        }

        u32 vsync_count =
            resources->graphics->wgpu.surface_caps.presentModeCount;
        const WGPUPresentMode *modes =
            resources->graphics->wgpu.surface_caps.presentModes;
        u32 current_index;
        for (current_index = 0; current_index < vsync_count; current_index++)
        {
            WGPUPresentMode mode = modes[current_index];
            if (current_mode == mode)
                break;
        }
        char *vsync_text = text_of_vsync_mode(current_mode);
        draw_text_selector_to(menu->category_surf, category_font, 180, 2,
                              vsync_text, 10);

        i32 button_x = 180;
        i32 button_y = 2;
        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && mouse_clicked)
        {
            if (current_index == 0)
                current_index = vsync_count - 1;
            else
                current_index--;
            resources->settings->video.present_mode = modes[current_index];
        }

        // < + " " + 10 + " "
        button_x = button_x + (character_width * 13);
        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && mouse_clicked)
        {
            if (current_index == vsync_count - 1)
                current_index = 0;
            else
                current_index++;
            resources->settings->video.present_mode = modes[current_index];
        }

        WGPUPresentMode new_mode = resources->settings->video.present_mode;
        bool did_change_mode = new_mode != current_mode;
        if (did_change_mode)
        {
            resources->graphics->wgpu.surface_config.presentMode = new_mode;
            wgpuSurfaceConfigure(resources->graphics->wgpu.surface,
                                 &resources->graphics->wgpu.surface_config);
        }

        break;
    }
    default:
        break;
    }

    WGPUTexture texture = texture_manager_get_texture(
        &resources->graphics->texture_manager, menu->category.texture);
    write_surface_to_texture(menu->category_surf, texture,
                             &resources->graphics->wgpu);
}

void settings_menu_free(SettingsMenu *menu, Resources *resources)
{
    ui_sprite_free(&menu->background, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.middle, menu->bg_entry);

    ui_sprite_free(&menu->category, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.foreground,
                 menu->category_entry);

    for (int i = 0; i < 4; i++)
    {
        ui_sprite_free(&menu->categories[i], resources->graphics);
        layer_remove(&resources->graphics->ui_layers.foreground,
                     menu->cat_entries[i]);
    }

    SDL_DestroySurface(menu->category_surf);
}
