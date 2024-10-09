#include "settings.h"
#include "utility/macros.h"
#include <stdio.h>

void settings_load_from(Settings *settings, const char *path,
                        SDL_Window *window, WGPUResources *resources)
{
    settings->audio.bgm_volume = 100;
    settings->audio.sfx_volume = 100;

    settings->video.frame_cap = false;
    settings->video.max_framerate = 0;
    settings->video.present_mode = resources->surface_config.presentMode;
    settings->video.fullscreen = false;

    settings->keybinds.up = SDLK_UP;
    settings->keybinds.left = SDLK_LEFT;
    settings->keybinds.right = SDLK_RIGHT;
    settings->keybinds.down = SDLK_DOWN;

    settings->keybinds.jump = SDLK_SPACE;
    settings->keybinds.crouch = SDLK_C;

    settings->keybinds.back = SDLK_ESCAPE;
    settings->keybinds.quit = SDLK_Q;
}

void settings_save_to(Settings *settings, const char *path)
{
    FILE *file = fopen(path, "w");
    PTR_ERRCHK(file, "failed to open settings file");

    fprintf(file, "[audio]\n");
    fprintf(file, "bgm_volume = %d\n", settings->audio.bgm_volume);
    fprintf(file, "sfx_volume = %d\n", settings->audio.sfx_volume);
    fprintf(file, "\n");

    fprintf(file, "[video]\n");
    fprintf(file, "frame_cap = %d\n", settings->video.frame_cap);
    fprintf(file, "max_framerate = %d\n", settings->video.max_framerate);
    fprintf(file, "present_mode = %d\n", settings->video.present_mode);
    fprintf(file, "fullscreen = %d\n", settings->video.fullscreen);
    fprintf(file, "\n");

    fprintf(file, "[keybinds]\n");
    fprintf(file, "up = %d\n", settings->keybinds.up);
    fprintf(file, "left = %d\n", settings->keybinds.left);
    fprintf(file, "right = %d\n", settings->keybinds.right);
    fprintf(file, "down = %d\n", settings->keybinds.down);
    fprintf(file, "jump = %d\n", settings->keybinds.jump);
    fprintf(file, "crouch = %d\n", settings->keybinds.crouch);
    fprintf(file, "back = %d\n", settings->keybinds.back);
    fprintf(file, "quit = %d\n", settings->keybinds.quit);

    fclose(file);
}
