#include "map.h"
#include "box2d/collision.h"
#include "box2d/math_functions.h"
#include "cglm/struct/vec2.h"
#include "core_types.h"
#include "debug/level_editor.h"
#include "graphics/quad_manager.h"
#include "graphics/tex_manager.h"
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

static void handle_tile_layers(Map *map, MapLayer *layer, u32 *layer_index,
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
            handle_tile_layers(map, &layer->data.group.layers[i], layer_index,
                               tile_ids);
    }
}

static void handle_object_layers(MapLayer *layer, Resources *resources,
                                 b2Vec2 *initial_player_pos)
{
    if (layer->type == Layer_Group)
    {
        for (u32 i = 0; i < layer->data.group.layer_len; i++)
            handle_object_layers(&layer->data.group.layers[i], resources,
                                 initial_player_pos);
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
                    (b2Vec2){(object->x + object->width / 2) / PX_PER_M,
                             -(object->y + object->height / 2) / PX_PER_M};

                b2BodyId groundId =
                    b2CreateBody(resources->physics->world, &groundBodyDef);
                b2Polygon groundBox = b2MakeBox(object->width / PX_PER_M / 2,
                                                object->height / PX_PER_M / 2);
                b2ShapeDef groundShapeDef = b2DefaultShapeDef();
                b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);
                break;
            }
            case Obj_Ellipse:
            {
                // TODO actually support ellipses
                b2BodyDef groundBodyDef = b2DefaultBodyDef();
                groundBodyDef.position =
                    (b2Vec2){(object->x + object->width / 2) / PX_PER_M,
                             -(object->y + object->width / 2) / PX_PER_M};

                b2BodyId groundId =
                    b2CreateBody(resources->physics->world, &groundBodyDef);
                b2Circle groundCircle = {
                    .center = {0, 0},
                    .radius = object->width / PX_PER_M / 2,
                };
                b2ShapeDef groundShapeDef = b2DefaultShapeDef();
                b2CreateCircleShape(groundId, &groundShapeDef, &groundCircle);
                break;
            }
            case Obj_Polygon:
            {
                b2BodyDef groundBodyDef = b2DefaultBodyDef();
                groundBodyDef.position =
                    (b2Vec2){(object->x + object->width / 2) / PX_PER_M,
                             -(object->y + object->height / 2) / PX_PER_M};

                b2BodyId groundId =
                    b2CreateBody(resources->physics->world, &groundBodyDef);

                // we have to scale down the polygon vertices to the physics
                // scale
                // we should proooooooobably restore the original values after
                // but ehhhhh
                for (u32 j = 0; j < object->polygon_len; j++)
                {
                    object->polygons[j].x /= PX_PER_M;
                    object->polygons[j].y /= -PX_PER_M;
                }

                b2Hull groundHull =
                    b2ComputeHull(object->polygons, object->polygon_len);
                b2Polygon groundPolygon = b2MakePolygon(&groundHull, 0.0);
                b2ShapeDef groundShapeDef = b2DefaultShapeDef();
                b2CreatePolygonShape(groundId, &groundShapeDef, &groundPolygon);
                break;
            }
            case Obj_Point:
            {
                if (strcmp(object->name, "spawnpoint") == 0)
                    *initial_player_pos =
                        (b2Vec2){object->x / PX_PER_M, -object->y / PX_PER_M};
                break;
            }
            default:
                break;
            }
        }
    }
}

static void handle_image_layers(MapLayer *layer, Resources *resources)
{
    if (layer->type == Layer_Group)
    {
        for (u32 i = 0; i < layer->data.group.layer_len; i++)
            handle_image_layers(&layer->data.group.layers[i], resources);
    }

    if (layer->type == Layer_Image)
    {
        TextureEntry *texture_entry = texture_manager_load(
            &resources->graphics->texture_manager, layer->data.image.image_path,
            &resources->graphics->wgpu);
        WGPUTexture texture = texture_manager_get_texture(
            &resources->graphics->texture_manager, texture_entry);

        Transform transform = transform_from_xyz(layer->data.image.offset_x,
                                                 layer->data.image.offset_y, 0);
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics->transform_manager, transform);

        Rect rect = rect_from_min_size(
            GLMS_VEC2_ZERO, (vec2s){.x = wgpuTextureGetWidth(texture),
                                    .y = wgpuTextureGetHeight(texture)});
        Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
        Quad quad = {.rect = rect, .tex_coords = tex_coords};

        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics->quad_manager, quad);

        Sprite *sprite = malloc(sizeof(Sprite));
        sprite_init(sprite, texture_entry, transform_entry, quad_entry);
        layer_add(&resources->graphics->sprite_layers.background, sprite);
    }
}

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

    char out_err_msg[256];
    INFF *inff = inff_parse(args->map_path, out_err_msg);
    if (!inff)
        FATAL("Failed to parse INFF file: %s", out_err_msg);

    Map map;
    parse_map_from(&map, inff);
    inff_free(inff); // map copies inff data, so we can free it now

    b2Vec2 initial_player_pos = {0, 0};
    for (u32 i = 0; i < map.layer_len; i++)
    {
        handle_object_layers(&map.layers[i], resources, &initial_player_pos);
    }

    for (u32 i = 0; i < map.layer_len; i++)
    {
        handle_image_layers(&map.layers[i], resources);
    }

    {
        Transform transform = transform_from_xyz(0, 0, 0);
        TransformEntry tilemap_transform = transform_manager_add(
            &resources->graphics->transform_manager, transform);
        TextureEntry *tileset = texture_manager_load(
            &resources->graphics->texture_manager, map.tileset.image_path,
            &resources->graphics->wgpu);

        u32 tile_layer_count = 0;
        for (u32 i = 0; i < map.layer_len; i++)
            count_tile_map_layers(&map.layers[i], &tile_layer_count);

        i32 *map_data =
            malloc(sizeof(i32) * map.width * map.height * tile_layer_count);

        u32 tile_layer_index = 0;
        for (u32 i = 0; i < map.layer_len; i++)
            handle_tile_layers(&map, &map.layers[i], &tile_layer_index,
                               map_data);

        tilemap_init(&map_scene->tilemap, resources->graphics, tileset,
                     tilemap_transform, map.width, map.height, tile_layer_count,
                     map_data);

        free(map_data);

        TilemapLayer *background = malloc(sizeof(TilemapLayer));
        background->tilemap = &map_scene->tilemap;
        background->layer = 0;
        layer_add(&resources->graphics->tilemap_layers.middle, background);
    }

    map_free(&map);

    player_init(&map_scene->player, initial_player_pos, resources);
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
