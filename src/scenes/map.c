#include "map.h"
#include "graphics/tilemap.h"
#include "input/input.h"
#include "player.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"

void map_scene_init(Scene **scene_data, Resources *resources)
{
    MapScene *map_scene = malloc(sizeof(MapScene));
    map_scene->type = Scene_Map;
    *scene_data = (Scene *)map_scene;

    map_scene->freecam = false;
    map_scene->level_editor_enabled = false;

    {
        Transform transform = transform_from_xyz(0, 0, 0);
        TransformEntry tilemap_transform = transform_manager_add(
            &resources->graphics->transform_manager, transform);
        TextureEntry *tileset = texture_manager_load(
            &resources->graphics->texture_manager,
            "assets/textures/red_start.png", &resources->graphics->wgpu);
        u32 map_data[50 * 5];
        for (int i = 0; i < 50 * 5; i++)
        {
            map_data[i] = 1;
        }
        tilemap_init(&map_scene->tilemap, resources->graphics, tileset,
                     tilemap_transform, 50, 5, 1, map_data);

        TilemapLayer *background = malloc(sizeof(TilemapLayer));
        background->tilemap = &map_scene->tilemap;
        background->layer = 0;
        layer_add(&resources->graphics->tilemap_layers.background, background);
    }

    player_init(&map_scene->player, resources);

    lvledit_init(resources->graphics, &map_scene->tilemap);
}

void map_scene_update(Scene *scene_data, Resources *resources)
{
    MapScene *map_scene = (MapScene *)scene_data;

    if (input_is_pressed(resources->input, Button_Freecam))
        map_scene->freecam = !map_scene->freecam;

    player_update(&map_scene->player, resources, map_scene->freecam);

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
