#include "settings.h"
#include "utility/macros.h"
#include "parsers/ini.h"
#include <stdio.h>
#include <sys/stat.h>

void settings_load_from(Settings *settings, const char *path,
                        SDL_Window *window, WGPUResources *resources)
{
    struct stat buffer;
    bool exists = stat(path, &buffer) == 0;

    Ini *ini = NULL;
    if (exists)
    {
        char out_err_msg[256];
        ini = ini_parse_file(path, out_err_msg);
        if (!ini)
            fprintf(stderr, "error loading %s: %s", path, out_err_msg);
    }

    if (ini)
    {
        for (i32 i = 0; i < ini->sections->len; i++)
        {
#define SET_VALUE_FROM_PAIR(pair, category, ini_key)                           \
    if (strcmp(pair->key, #ini_key) == 0)                                      \
    {                                                                          \
        category.ini_key = atoi(pair->value);                                  \
        continue;                                                              \
    }

            IniSection *section = linked_list_at(ini->sections, i);
            if (strcmp(section->name, "audio") == 0)
            {
                for (i32 i = 0; i < section->pairs->len; i++)
                {
                    IniPair *pair = linked_list_at(section->pairs, i);
                    SET_VALUE_FROM_PAIR(pair, settings->audio, bgm_volume);
                    SET_VALUE_FROM_PAIR(pair, settings->audio, sfx_volume);

                    fprintf(stderr, "unrecognized ini field %s\n", pair->key);
                }

                continue;
            }
            if (strcmp(section->name, "video") == 0)
            {
                for (i32 i = 0; i < section->pairs->len; i++)
                {
                    IniPair *pair = linked_list_at(section->pairs, i);
                    SET_VALUE_FROM_PAIR(pair, settings->video, frame_cap);
                    SET_VALUE_FROM_PAIR(pair, settings->video, max_framerate);
                    SET_VALUE_FROM_PAIR(pair, settings->video, present_mode);
                    SET_VALUE_FROM_PAIR(pair, settings->video, fullscreen);

                    fprintf(stderr, "unrecognized ini field %s\n", pair->key);
                }

                continue;
            }
            if (strcmp(section->name, "keybinds") == 0)
            {
                for (i32 i = 0; i < section->pairs->len; i++)
                {
                    IniPair *pair = linked_list_at(section->pairs, i);
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, up);
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, left);
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, right);
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, down);

                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, jump);
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, crouch);

                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, back);
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, quit);

                    fprintf(stderr, "unrecognized ini field %s\n", pair->key);
                }

                continue;
            }

            fprintf(stderr, "unrecognized ini section %s\n", section->name);
        }
    }
    else
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
    ini_free(ini);
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
