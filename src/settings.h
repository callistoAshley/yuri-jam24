#pragma once

#include <SDL3/SDL.h>
#include "sensible_nums.h"
#include <stdbool.h>
#include <wgpu.h>

typedef struct
{
    struct
    {
        u32 sfx_volume, bgm_volume;
    } audio;
    struct
    {
        WGPUPresentMode present_mode;
        bool frame_cap;
        u32 max_framerate;
        bool fullscreen;
    } video;
    struct
    {
        SDL_Keycode left, right, up, down;
        SDL_Keycode jump, cancel, interact;
        SDL_Keycode back, quit;
    } keybinds;
} Settings;

// loads settings, or uses defaults if possible
void settings_load_from(Settings *settings, u32 default_framerate,
                        const char *path);
void settings_save_to(Settings *settings, const char *path);
