#include "map.h"
#include "debug/level_editor.h"
#include "graphics/tilemap.h"
#include "input/input.h"
#include "player.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"
#include "parsers/inff.h"
#include "parsers/map.h"

// TODO add general map layer function, these all do the same thing
static void count_tile_map_layers(MapLayer *layer, u32 *tile_layer_count)
{
    if (layer->type == Layer_Tile)
    {
        (*tile_layer_count)++;
    }
    else if (layer->type == Layer_Group)
    {
        for (u32 i = 0; i < layer->data.group.layer_len; i++)
            count_tile_map_layers(&layer->data.group.layers[i],
                                  tile_layer_count);
    }
}

static void copy_tile_layer_data(Map *map, MapLayer *layer, u32 *layer_index,
                                 i32 *tile_ids)
{
    if (layer->type == Layer_Tile)
    {
        memcpy(&tile_ids[map->width * map->height * (*layer_index)],
               layer->data.tile.tiles, sizeof(i32) * map->width * map->height);
        (*layer_index)++;
    }
    else if (layer->type == Layer_Group)
    {
        for (u32 i = 0; i < layer->data.group.layer_len; i++)
            copy_tile_layer_data(map, &layer->data.group.layers[i], layer_index,
                                 tile_ids);
    }
}

static void add_collisions(MapLayer *layer, Physics *physics)
{
    if (layer->type == Layer_Group)
    {
        for (u32 i = 0; i < layer->data.group.layer_len; i++)
            add_collisions(&layer->data.group.layers[i], physics);
    }

    if (layer->type == Layer_Object)
    {
        for (u32 i = 0; i < layer->data.object.object_len; i++)
        {
            Object *object = &layer->data.object.objects[i];
            switch (object->type)
            {
            case Obj_Rectangle:
            {
                b2BodyDef groundBodyDef = b2DefaultBodyDef();
                groundBodyDef.position =
                    (b2Vec2){(object->height / 2 - object->x) / PX_PER_M + 0.5,
                             -(object->y + object->width / 2) / PX_PER_M + 0.5};

                b2BodyId groundId =
                    b2CreateBody(physics->world, &groundBodyDef);
                b2Polygon groundBox = b2MakeBox(object->width / PX_PER_M / 2,
                                                object->height / PX_PER_M / 2);
                b2ShapeDef groundShapeDef = b2DefaultShapeDef();
                b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);
                break;
            }
            default:
                break;
            }
        }
    }
}

void map_scene_init(Scene **scene_data, Resources *resources)
{
    MapScene *map_scene = malloc(sizeof(MapScene));
    map_scene->type = Scene_Map;
    *scene_data = (Scene *)map_scene;

    map_scene->freecam = false;

    char out_err_msg[256];
    INFF *inff = inff_parse("assets/maps/untitled.mnff", out_err_msg);
    if (!inff)
        FATAL("Failed to parse INFF file: %s", out_err_msg);

    Map map;
    parse_map_from(&map, inff);
    inff_free(inff); // map copies inff data, so we can free it now

    for (u32 i = 0; i < map.layer_len; i++)
    {
        add_collisions(&map.layers[i], resources->physics);
    }

    {
        Transform transform = transform_from_xyz(0, 0, 0);
        TransformEntry tilemap_transform = transform_manager_add(
            &resources->graphics->transform_manager, transform);
        TextureEntry *tileset = texture_manager_load(
            &resources->graphics->texture_manager,
            "assets/textures/red_start.png", &resources->graphics->wgpu);

        u32 tile_layer_count;
        for (u32 i = 0; i < map.layer_len; i++)
            count_tile_map_layers(&map.layers[i], &tile_layer_count);

        i32 *map_data =
            malloc(sizeof(i32) * map.width * map.height * tile_layer_count);

        u32 tile_layer_index = 0;
        for (u32 i = 0; i < map.layer_len; i++)
            copy_tile_layer_data(&map, &map.layers[i], &tile_layer_index,
                                 map_data);

        tilemap_init(&map_scene->tilemap, resources->graphics, tileset,
                     tilemap_transform, map.width, map.height, tile_layer_count,
                     map_data);

        TilemapLayer *background = malloc(sizeof(TilemapLayer));
        background->tilemap = &map_scene->tilemap;
        background->layer = 0;
        layer_add(&resources->graphics->tilemap_layers.background, background);
    }

    // map_free(&map);

    player_init(&map_scene->player, resources);
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
