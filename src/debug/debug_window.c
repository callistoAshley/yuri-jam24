#include "debug_window.h"
#include "scenes/map.h"

static int new_map_input_callback(ImGuiInputTextCallbackData *data)
{
    char pattern[512];
    int glob_count;
    char **files;

    snprintf(pattern, sizeof(pattern), "%s*.tmx", data->Buf);
    files = SDL_GlobDirectory("assets/maps/", pattern, 0, &glob_count);
    ;

    if (glob_count > 1)
    {
        // this is the pointer arithmetic ever. not commenting it. sorry
        size_t len = strlen(files[0]);
        for (int i = 1; i < glob_count; i++)
        {
            char *str = files[i];
            for (int j = 0; j < fmin(strlen(files[i]), len); j++)
            {
                if (*(str + j) != *(files[0] + j))
                {
                    len = j;
                    break;
                }
            }
        }
        ImGuiInputTextCallbackData_DeleteChars(data, 0, data->BufTextLen);
        ImGuiInputTextCallbackData_InsertChars(data, 0, files[0],
                                               files[0] + len);
    }
    else if (glob_count)
    {
        ImGuiInputTextCallbackData_DeleteChars(data, 0, data->BufTextLen);
        ImGuiInputTextCallbackData_InsertChars(data, 0, files[0],
                                               strstr(files[0], ".tmx"));
    }

    SDL_free(files);

    return 1;
}

void debug_wnd_show(DebugWindowState *state)
{
    if (igBegin("Debug", NULL, 0))
    {
        igCheckbox("Physics Draw", &state->resources->physics->debug_draw);
        // check if current scene is map
        if (state->resources->current_scene_interface->init == map_scene_init)
        {
            MapScene *map = (MapScene *)(*state->resources->current_scene);
            igCheckbox("Freecam", &map->freecam);
        }
        igSeparator();
        igInputText("New Map", state->new_map, sizeof(state->new_map),
                    ImGuiInputTextFlags_CallbackCompletion,
                    new_map_input_callback, NULL);
        if (igSmallButton("Change Map"))
        {
            char path[512];
            snprintf(path, 512, "assets/maps/%s.tmx", state->new_map);
            MapInitArgs args = {.map_path = path, .copy_map_path = true};
            scene_change(MAP_SCENE, state->resources, &args);
        }

        f32 speed = state->resources->time.virt->relative_speed;
        igSliderFloat("Relative Speed", &speed, 0.0, 10.0, "%f", 0);
        state->resources->time.virt->relative_speed = speed;

        igCheckbox("Pause", &state->resources->time.virt->paused);

        if (speed != 1.0)
        {
            if (igSmallButton("Reset"))
            {
                state->resources->time.virt->relative_speed = 1.0;
            }
        }

        f32 delta = time_delta_seconds(state->resources->time.real->time);
        igLabelText("FPS", "%f", 1.0 / delta);
    }
    igEnd();
}
