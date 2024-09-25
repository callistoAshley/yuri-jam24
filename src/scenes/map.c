#include "map.h"
#include "core_types.h"
#include "graphics/tilemap.h"
#include "input/input.h"
#include "player.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"

FMOD_STUDIO_EVENTDESCRIPTION *test_bgm;
FMOD_STUDIO_EVENTINSTANCE *test_bgm_instance;
FMOD_RESULT fr;

void map_scene_init(Scene **scene_data, Resources *resources, void *extra_args)
{
    MapInitArgs *args = (MapInitArgs *)extra_args;

    MapScene *map_scene = malloc(sizeof(MapScene));
    map_scene->type = Scene_Map;
    *scene_data = (Scene *)map_scene;

    map_scene->freecam = false;

    player_init(&map_scene->player, (b2Vec2){0, 0}, resources);
}

void map_scene_update(Scene *scene_data, Resources *resources)
{
    MapScene *map_scene = (MapScene *)scene_data;

    if (input_is_pressed(resources->input, Button_Freecam))
        map_scene->freecam = !map_scene->freecam;

    player_update(&map_scene->player, resources, map_scene->freecam);

    FMOD_Studio_EventInstance_SetParameterByName(
        test_bgm_instance, "test",
        map_scene->player.transform.position.x / 300.0f, false);

    if (map_scene->freecam)
    {
        bool left_down = input_is_down(resources->input, Button_Left);
        bool right_down = input_is_down(resources->input, Button_Right);
        bool up_down = input_is_down(resources->input, Button_Up);
        bool down_down = input_is_down(resources->input, Button_Down);

        const float freecam_move_speed =
            M_TO_PX(16) * resources->input->delta_seconds;
        if (right_down)
            resources->raw_camera->x += freecam_move_speed;
        if (left_down)
            resources->raw_camera->x -= freecam_move_speed;
        if (up_down)
            resources->raw_camera->y -= freecam_move_speed;
        if (down_down)
            resources->raw_camera->y += freecam_move_speed;
    }
    else
    {
        resources->raw_camera->x = map_scene->player.transform.position.x -
                                   INTERNAL_SCREEN_WIDTH / 2.0;
        resources->raw_camera->y = map_scene->player.transform.position.y -
                                   INTERNAL_SCREEN_HEIGHT / 2.0;
    }
}

void map_scene_free(Scene *scene_data, Resources *resources)
{
    MapScene *map_scene = (MapScene *)scene_data;
    tilemap_free(&map_scene->tilemap, resources->graphics);
    player_free(&map_scene->player, resources);
    free(map_scene);
}

const SceneInterface MAP_SCENE = {
    .init = map_scene_init,
    .update = map_scene_update,
    .free = map_scene_free,
};
