#include "settings.h"
#include "fmod_studio_common.h"
#include "fonts/font.h"
#include "graphics/tex_manager.h"
#include "resources.h"
#include "utility/common_defines.h"
#include "utility/graphics.h"
#include "utility/macros.h"
#include "utility/time.h"
#include "webgpu.h"

void settings_menu_init(SettingsMenu *menu, Resources *resources)
{
    {
        Quad quad = {
            .rect = rect_from_size(
                (vec2s){.x = GAME_VIEW_WIDTH, .y = GAME_VIEW_HEIGHT}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics.quad_manager, quad);

        Transform transform = transform_from_scale(VEC3_SPLAT(UI_SCALE));
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics.transform_manager, transform);

        TextureEntry *background_texture = texture_manager_load(
            &resources->graphics.texture_manager,
            TEXTURE_PATH("settings_background.png"), &resources->graphics.wgpu);

        ui_sprite_init(&menu->background, background_texture, transform_entry,
                       quad_entry, 0.0f);
        menu->bg_entry =
            layer_add(&resources->graphics.ui_layers.middle, &menu->background);
    }

    FMOD_Studio_System_GetEvent(resources->audio.system, "event:/menu/hover",
                                &menu->hover_desc);
    FMOD_Studio_System_GetEvent(resources->audio.system, "event:/menu/click",
                                &menu->click_desc);

    const char *categories[] = {
        "Audio",
        "Video",
        "Controls",
        "Back",
    };
    f32 start_y = 120 * UI_SCALE;
    SDL_Color color = {255, 255, 255, 255};

    u32 max_width = 0;
    for (int i = 0; i < 4; i++)
    {
        const char *category = categories[i];
        WGPUTexture texture =
            font_render_text(&resources->fonts.compaq.medium, category, color,
                             &resources->graphics.wgpu);

        char *path = malloc(strlen(category) + strlen("settings_option") + 1);
        strcpy(path, "settings_option");
        strcat(path, category);
        TextureEntry *texture_entry = texture_manager_register(
            &resources->graphics.texture_manager, texture, path);
        free(path);

        u32 width = wgpuTextureGetWidth(texture);
        u32 height = wgpuTextureGetHeight(texture);

        if (width > max_width)
            max_width = width;

        Transform transform = transform_from_xyz(15, start_y, 0);
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics.transform_manager, transform);
        start_y += height + 5;

        Quad quad = {
            .rect = rect_from_size((vec2s){.x = width, .y = height}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics.quad_manager, quad);

        ui_sprite_init(&menu->categories[i], texture_entry, transform_entry,
                       quad_entry, 0.0f);
        menu->cat_entries[i] = layer_add(
            &resources->graphics.ui_layers.foreground, &menu->categories[i]);
    }

    {
        WGPUTexture texture = blank_texture(600, 200,
                                            WGPUTextureUsage_CopyDst |
                                                WGPUTextureUsage_TextureBinding,
                                            &resources->graphics.wgpu);
        TextureEntry *texture_entry =
            texture_manager_register(&resources->graphics.texture_manager,
                                     texture, "settings_menu_category");

        Transform transform =
            transform_from_xyz(15 + max_width + 15, 120 * UI_SCALE, 0);
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics.transform_manager, transform);

        Quad quad = {
            .rect = rect_from_size((vec2s){.x = 600, .y = 200}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics.quad_manager, quad);

        ui_sprite_init(&menu->category, texture_entry, transform_entry,
                       quad_entry, 0.0f);
        menu->category_entry = layer_add(
            &resources->graphics.ui_layers.foreground, &menu->category);
    }
    menu->category_surf = SDL_CreateSurface(600, 200, SDL_PIXELFORMAT_RGBA32);

    menu->hovered_category = Cat_None;
    menu->selected_category = Cat_None;

    menu->waiting_on_keybind = -1;

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
                                  i32 y, const char *value)
{
    char text[40];
    snprintf(text, sizeof(text), "< %s >", value);

    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface *src = font_render_surface(font, text, color);
    SDL_Rect dst_rect = {.x = x, .y = y, .w = src->w, .h = src->h};
    SDL_BlitSurface(src, NULL, surface, &dst_rect);
    SDL_DestroySurface(src);
}

static void draw_checked_text_to(SDL_Surface *surface, Font *font, i32 x, i32 y,
                                 const char *value, bool checked)
{
    char text[40];
    if (checked)
        snprintf(text, sizeof(text), "<%s>", value);
    else
        snprintf(text, sizeof(text), "-%s-", value);

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
        return "ON";
    case WGPUPresentMode_FifoRelaxed:
        return "Adaptive";
    case WGPUPresentMode_Immediate:
        return "OFF";
    case WGPUPresentMode_Mailbox:
        return "Fast";
    // this should NEVER happen
    default:
        return "oops";
    }
}

void settings_menu_update(SettingsMenu *menu, Resources *resources)
{
    if (!menu->open)
        return;

    Settings *settings = &resources->settings;

    f32 delta = duration_as_secs(resources->time.real.time.delta);
    bool is_opening = menu->background.opacity < 1.0f && !menu->is_closing;
    if (is_opening)
    {
        menu->background.opacity += 10.0f * delta;
        menu->background.opacity = fminf(menu->background.opacity, 1.0f);

        for (int i = 0; i < 4; i++)
        {
            menu->categories[i].opacity = menu->background.opacity / 2.0;
        }

        menu->waiting_on_keybind = -1;

        resources->time.virt.paused = true;

        return;
    }

    if (menu->is_closing)
    {
        menu->background.opacity -= 10.0f * delta;
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

            resources->time.virt.paused = false;

            return;
        }
    }

    u32 max_width = 0;

    bool did_hover_option = false;
    bool hovered_new_option = false;

    f32 mouse_x =
        resources->input.mouse_x * resources->input.mouse_scale_factor;
    f32 mouse_y =
        resources->input.mouse_y * resources->input.mouse_scale_factor;
    f32 start_y = 120 * UI_SCALE;

    for (SettingsCategory i = 0; i < 4; i++)
    {
        UiSprite *category = &menu->categories[i];

        WGPUTexture texture = texture_manager_get_texture(
            &resources->graphics.texture_manager, category->texture);
        u32 width = wgpuTextureGetWidth(texture);
        u32 height = wgpuTextureGetHeight(texture);

        f32 sprite_y = start_y;
        start_y += height + 5;

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

    if (!did_hover_option)
    {
        menu->hovered_category = Cat_None;
    }

    // fire and forget event instances
    // FMOD will automatically release the instance when it's finished
    if (hovered_new_option)
        fire_and_forget(menu->hover_desc);

    bool is_waiting_on_key = menu->waiting_on_keybind != -1;
    if (input_did_press(&resources->input, Button_Back) && !is_opening &&
        !is_waiting_on_key)
    {
        menu->is_closing = true;
    }

    bool mouse_clicked = input_did_press(&resources->input, Button_MouseLeft);

    bool did_select_option =
        menu->hovered_category != Cat_None &&
        menu->selected_category != menu->hovered_category && mouse_clicked;
    if (did_select_option && !is_waiting_on_key)
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

    Font *category_font = &resources->fonts.compaq.medium;

    i32 character_width, character_height;
    font_texture_size(category_font, " ", &character_width, &character_height);

    i32 relative_mouse_x = mouse_x - (15 + max_width + 15);
    i32 relative_mouse_y = mouse_y - (120 * UI_SCALE);

    bool mouse_down = input_is_down(&resources->input, Button_MouseLeft);

    // TODO: add repeat logic to input
    if (mouse_clicked)
        menu->repeat_input_timer = 0.5;

    if (mouse_down)
        menu->repeat_input_timer -= delta;

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
        i32 button_x = 180;
        i32 button_y = 2;
        // bgm volume
        draw_text_at(menu->category_surf, category_font, 0, 0, "BGM Volume");
        // bgm volume number
        draw_number_selector_to(menu->category_surf, category_font, button_x,
                                button_y, settings->audio.bgm_volume);

        bool did_change_bgm_volume = false;
        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && repeat_clicked)
        {

            if (settings->audio.bgm_volume > 0)
            {
                settings->audio.bgm_volume--;
                fire_and_forget(menu->hover_desc);
                did_change_bgm_volume = true;
            }
        }

        button_x = button_x + (character_width * 6);
        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && repeat_clicked)
        {
            if (settings->audio.bgm_volume < 100)
            {
                settings->audio.bgm_volume++;
                fire_and_forget(menu->hover_desc);
                did_change_bgm_volume = true;
            }
        }

        button_x = 180;
        button_y = character_height + 5 + 2;
        // sfx volume
        draw_text_at(menu->category_surf, category_font, 0,
                     character_height + 5, "SFX Volume");
        // sfx volume number
        draw_number_selector_to(menu->category_surf, category_font, button_x,
                                button_y, settings->audio.sfx_volume);

        bool did_change_sfx_volume = false;
        if (MOUSE_INSIDE_BUTTON(button_x, button_y))
        {
            if (repeat_clicked)
            {
                if (settings->audio.sfx_volume > 0)
                {
                    settings->audio.sfx_volume--;
                    fire_and_forget(menu->hover_desc);
                    did_change_sfx_volume = true;
                }
            }
        }

        button_x = button_x + (character_width * 6);
        if (MOUSE_INSIDE_BUTTON(button_x, button_y))
        {
            if (repeat_clicked)
            {
                if (settings->audio.sfx_volume < 100)
                {
                    settings->audio.sfx_volume++;
                    fire_and_forget(menu->hover_desc);
                    did_change_sfx_volume = true;
                }
            }
        }

        if (did_change_bgm_volume)
        {
            FMOD_Studio_Bus_SetVolume(resources->audio.bgm_bus,
                                      settings->audio.bgm_volume / 100.0);
        }

        if (did_change_sfx_volume)
        {
            FMOD_Studio_Bus_SetVolume(resources->audio.sfx_bus,
                                      settings->audio.sfx_volume / 100.0);
        }

        break;
    }
    case Cat_Video:
    {
        SDL_DisplayID display_id = SDL_GetDisplayForWindow(resources->window);
        const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display_id);
        SDL_PTR_ERRCHK(mode, "failed to get monitor display mode");
        u32 max_framerate = mode->refresh_rate;

        // vsync
        draw_text_at(menu->category_surf, category_font, 0, 0, "Vsync");

        i32 button_x = 180;
        i32 button_y = 2;
        WGPUPresentMode current_mode = settings->video.present_mode;
        u32 vsync_count =
            resources->graphics.wgpu.surface_caps.presentModeCount;
        const WGPUPresentMode *modes =
            resources->graphics.wgpu.surface_caps.presentModes;
        u32 current_index;
        for (current_index = 0; current_index < vsync_count; current_index++)
        {
            WGPUPresentMode mode = modes[current_index];
            if (current_mode == mode)
                break;
        }
        char *vsync_text = text_of_vsync_mode(current_mode);
        u32 vsync_len = strlen(vsync_text);
        draw_text_selector_to(menu->category_surf, category_font, button_x,
                              button_y, vsync_text);

        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && mouse_clicked)
        {
            if (current_index == 0)
                current_index = vsync_count - 1;
            else
                current_index--;
            settings->video.present_mode = modes[current_index];
        }

        // < + " " + vsync_len + " "
        button_x = button_x + character_width * (vsync_len + 3);
        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && mouse_clicked)
        {
            if (current_index == vsync_count - 1)
                current_index = 0;
            else
                current_index++;
            settings->video.present_mode = modes[current_index];
        }

        WGPUPresentMode new_mode = settings->video.present_mode;
        bool did_change_mode = new_mode != current_mode;
        if (did_change_mode)
        {
            resources->graphics.wgpu.surface_config.presentMode = new_mode;
            wgpuSurfaceConfigure(resources->graphics.wgpu.surface,
                                 &resources->graphics.wgpu.surface_config);
            fire_and_forget(menu->hover_desc);
        }

        draw_text_at(menu->category_surf, category_font, 0,
                     character_height + 5, "Fullscreen");

        char *fullscreen_text;
        if (settings->video.fullscreen)
            fullscreen_text = "ON";
        else
            fullscreen_text = "OFF";
        u32 fullscreen_len = strlen(fullscreen_text);
        draw_text_selector_to(menu->category_surf, category_font, 180,
                              2 + character_height + 5, fullscreen_text);

        button_x = 180;
        button_y = 2 + character_height + 5;
        bool fullscreen = settings->video.fullscreen;

        bool inside_fullscreen =
            MOUSE_INSIDE_BUTTON(button_x, button_y) && mouse_clicked;
        // < + " " + fullscreen_len + " "
        button_x = 180 + character_width * (fullscreen_len + 3);
        inside_fullscreen |=
            MOUSE_INSIDE_BUTTON(button_x, button_y) && mouse_clicked;

        if (inside_fullscreen)
        {
            settings->video.fullscreen = !fullscreen;
            SDL_SetWindowFullscreen(resources->window, !fullscreen);
            fire_and_forget(menu->hover_desc);
        }

        draw_text_at(menu->category_surf, category_font, 0,
                     (character_height + 5) * 2, "Max Framerate");

        i32 y = 2 + (character_height + 5) * 2;
        if (settings->video.frame_cap)
        {
            draw_number_selector_to(menu->category_surf, category_font, 220, y,
                                    settings->video.max_framerate);
        }
        else
        {
            draw_text_selector_to(menu->category_surf, category_font, 220, y,
                                  "Unlimited");
        }

        button_x = 220;
        button_y = 2 + (character_height + 5) * 2;

        if (MOUSE_INSIDE_BUTTON(button_x, button_y) && repeat_clicked)
        {
            if (settings->video.max_framerate > 30)
            {
                if (settings->video.frame_cap)
                    settings->video.max_framerate--;
                else
                    settings->video.max_framerate = max_framerate;
                settings->video.frame_cap = true;
                fire_and_forget(menu->hover_desc);
            }
        }

        if (settings->video.frame_cap)
        {
            // < + " " + 3 + " "
            button_x = 220 + character_width * 6;

            if (MOUSE_INSIDE_BUTTON(button_x, button_y) && repeat_clicked)
            {
                if (settings->video.max_framerate >= max_framerate)
                    settings->video.frame_cap = false;
                else
                    settings->video.max_framerate++;
                fire_and_forget(menu->hover_desc);
            }
        }

        break;
    }
    case Cat_Controls:
    {
        draw_text_at(menu->category_surf, category_font, 0, 0, "Directional");

        bool clicked_dir_button = MOUSE_INSIDE_BUTTON(190, 2) && mouse_clicked;
        if (settings->keybinds.left == SDLK_LEFT)
        {
            draw_text_selector_to(menu->category_surf, category_font, 190, 2,
                                  "Arrow Keys");

            i32 button_x = 190 + character_width * (strlen("Arrow Keys") + 3);
            clicked_dir_button |=
                MOUSE_INSIDE_BUTTON(button_x, 2) && mouse_clicked;

            if (clicked_dir_button)
            {
                settings->keybinds.left = SDLK_A;
                settings->keybinds.right = SDLK_D;
                settings->keybinds.up = SDLK_W;
                settings->keybinds.down = SDLK_D;
            }
        }
        else
        {
            draw_text_selector_to(menu->category_surf, category_font, 190, 2,
                                  "WASD");

            i32 button_x = 190 + character_width * (strlen("WASD") + 3);
            clicked_dir_button |=
                MOUSE_INSIDE_BUTTON(button_x, 2) && mouse_clicked;

            if (clicked_dir_button)
            {
                settings->keybinds.left = SDLK_LEFT;
                settings->keybinds.right = SDLK_RIGHT;
                settings->keybinds.up = SDLK_UP;
                settings->keybinds.down = SDLK_DOWN;
            }
        }

#define KEY_SELECTOR(text, index, name_x, name_y, selector_x, selector_y,      \
                     setting)                                                  \
    {                                                                          \
        draw_text_at(menu->category_surf, category_font, name_x, name_y,       \
                     text);                                                    \
                                                                               \
        const char *key_text = SDL_GetKeyName(setting);                        \
        i32 button_width = (strlen(key_text) + 2) * character_width;           \
        i32 button_height = character_height;                                  \
        draw_checked_text_to(menu->category_surf, category_font, selector_x,   \
                             selector_y, key_text,                             \
                             menu->waiting_on_keybind == index);               \
                                                                               \
        Rect button_rect =                                                     \
            rect_from_min_size((vec2s){.x = selector_x, selector_y},           \
                               (vec2s){.x = button_width, button_height});     \
        vec2s relative_mouse_pos = {.x = relative_mouse_x,                     \
                                    .y = relative_mouse_y};                    \
                                                                               \
        if (menu->waiting_on_keybind == index &&                               \
            resources->input.key_has_pressed)                                  \
        {                                                                      \
            setting = resources->input.last_pressed_key;                       \
            menu->waiting_on_keybind = -1;                                     \
        }                                                                      \
                                                                               \
        bool did_click_button =                                                \
            rect_contains(button_rect, relative_mouse_pos) && mouse_clicked && \
            !is_waiting_on_key;                                                \
        if (did_click_button)                                                  \
        {                                                                      \
            menu->waiting_on_keybind = index;                                  \
        }                                                                      \
    }

        i32 start_y = character_height + 5;
        KEY_SELECTOR("Jump", 0, 0, start_y, 140, start_y,
                     settings->keybinds.jump);
        i32 separator_width = character_width * 2;
        KEY_SELECTOR("| Cancel", 1, 280, start_y, 420 + separator_width,
                     start_y, settings->keybinds.cancel);

        start_y += character_height + 5;
        KEY_SELECTOR("Interact", 2, 0, start_y, 140, start_y,
                     settings->keybinds.interact);
        KEY_SELECTOR("| Back", 3, 280, start_y, 420 + separator_width, start_y,
                     settings->keybinds.back);

        start_y += character_height + 5;
        KEY_SELECTOR("Items", 4, 0, start_y, 140, start_y,
                     settings->keybinds.inventory);

        break;
    }
    default:
        break;
    }

    WGPUTexture texture = texture_manager_get_texture(
        &resources->graphics.texture_manager, menu->category.texture);
    write_surface_to_texture(menu->category_surf, texture,
                             &resources->graphics.wgpu);
}

void settings_menu_free(SettingsMenu *menu, Resources *resources)
{
    ui_sprite_free(&menu->background, &resources->graphics);
    layer_remove(&resources->graphics.ui_layers.middle, menu->bg_entry);

    ui_sprite_free(&menu->category, &resources->graphics);
    layer_remove(&resources->graphics.ui_layers.foreground,
                 menu->category_entry);

    for (int i = 0; i < 4; i++)
    {
        ui_sprite_free(&menu->categories[i], &resources->graphics);
        layer_remove(&resources->graphics.ui_layers.foreground,
                     menu->cat_entries[i]);
    }

    SDL_DestroySurface(menu->category_surf);
}
