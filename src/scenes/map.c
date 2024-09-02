#include "map.h"
#include "graphics/tilemap.h"
#include "player.h"

typedef struct
{
    Tilemap tilemap;
    Player player;
} MapScene;

void map_scene_init(void **scene_data, Resources *resources)
{
    MapScene *map_scene = malloc(sizeof(MapScene));
    *scene_data = map_scene;

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
}

void map_scene_update(void *scene_data, Resources *resources)
{
    MapScene *map_scene = scene_data;
    player_update(&map_scene->player, resources);
}

void map_scene_free(void *scene_data, Resources *resources)
{
    MapScene *map_scene = scene_data;
    tilemap_free(&map_scene->tilemap, resources->graphics);
    player_free(&map_scene->player, resources);
    free(map_scene);
}

const SceneInterface MAP_SCENE = {
    .init = map_scene_init,
    .update = map_scene_update,
    .free = map_scene_free,
};
