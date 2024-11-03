#include "settings.h"
#include "utility/macros.h"
#include "parsers/ini.h"
#include <stdio.h>
#include <sys/stat.h>

void settings_load_from(Settings *settings, u32 default_framerate,
                        const char *path)
{
    // set defaults
    settings->audio.bgm_volume = 100;
    settings->audio.sfx_volume = 100;

    settings->video.frame_cap = true;
    settings->video.max_framerate = default_framerate;
    settings->video.present_mode = WGPUPresentMode_FifoRelaxed;
    settings->video.fullscreen = false;

    settings->keybinds.up = SDLK_UP;
    settings->keybinds.left = SDLK_LEFT;
    settings->keybinds.right = SDLK_RIGHT;
    settings->keybinds.down = SDLK_DOWN;

    settings->keybinds.jump = SDLK_SPACE;
    settings->keybinds.cancel = SDLK_X;
    settings->keybinds.interact = SDLK_Z;

    settings->keybinds.back = SDLK_ESCAPE;
    settings->keybinds.quit = SDLK_Q;

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
        category.ini_key = atol(pair->value);                                  \
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

                    fprintf(stderr, "unrecognized ini field '%s'\n", pair->key);
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

                    fprintf(stderr, "unrecognized ini field '%s'\n", pair->key);
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
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, cancel);

                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, back);
                    SET_VALUE_FROM_PAIR(pair, settings->keybinds, quit);

                    fprintf(stderr, "unrecognized ini field '%s'\n", pair->key);
                }

                continue;
            }

            fprintf(stderr, "unrecognized ini section '%s'\n", section->name);
        }

        ini_free(ini);
    }
}

void settings_save_to(Settings *settings, const char *path)
{
    FILE *file = fopen(path, "w");
    PTR_ERRCHK(file, "failed to open settings file");

    fprintf(file, "[audio]\n");
    fprintf(file, "bgm_volume=%d\n", settings->audio.bgm_volume);
    fprintf(file, "sfx_volume=%d\n", settings->audio.sfx_volume);
    fprintf(file, "\n");

    fprintf(file, "[video]\n");
    fprintf(file, "frame_cap=%d\n", settings->video.frame_cap);
    fprintf(file, "max_framerate=%d\n", settings->video.max_framerate);
    fprintf(file, "present_mode=%d\n", settings->video.present_mode);
    fprintf(file, "fullscreen=%d\n", settings->video.fullscreen);
    fprintf(file, "\n");

    fprintf(file, "[keybinds]\n");
    fprintf(file, "up=%u\n", settings->keybinds.up);
    fprintf(file, "left=%u\n", settings->keybinds.left);
    fprintf(file, "right=%u\n", settings->keybinds.right);
    fprintf(file, "down=%u\n", settings->keybinds.down);
    fprintf(file, "jump=%u\n", settings->keybinds.jump);
    fprintf(file, "cancel=%u\n", settings->keybinds.cancel);
    fprintf(file, "back=%u\n", settings->keybinds.back);
    fprintf(file, "quit=%u\n", settings->keybinds.quit);

    fclose(file);
}
