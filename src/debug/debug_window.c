#include "debug_window.h"
#include "scenes/map.h"

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
        igInputText("New Map", state->new_map, sizeof(state->new_map), 0, NULL,
                    NULL);
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
    }
    igEnd();
}
