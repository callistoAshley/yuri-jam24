#include "map.h"
#include "core_types.h"
#include "graphics/light.h"
#include "graphics/tilemap.h"
#include "input/input.h"
#include "player.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"
#include "utility/log.h"
#include "utility/macros.h"

#include <tmx.h>

#define COLLISION_CLASS "collision"
#define LIGHTS_CLASS "lights"

FMOD_STUDIO_EVENTDESCRIPTION *test_bgm;
FMOD_STUDIO_EVENTINSTANCE *test_bgm_instance;
FMOD_RESULT fr;

static char *tiled_image_path_to_actual(char *path)
{
    const char *prefix = "assets/textures/";
    char *new_path = malloc(strlen(prefix) + strlen(path) + 1);
    strcpy(new_path, prefix);
    strcat(new_path, path);
    return new_path;
}

typedef struct
{
    i32 *tiles;
    u32 width, height, layers;
    Tilemap *tilemap;
} MapData;

static void handle_collision_layer(tmx_layer *layer, Resources *resources,
                                   b2Vec2 *player_position)
{
    tmx_object *current = layer->content.objgr->head;
    while (current)
    {
        switch (current->obj_type)
        {
        case OT_SQUARE:
        {
            b2BodyDef groundBodyDef = b2DefaultBodyDef();
            groundBodyDef.position =
                (b2Vec2){(current->x + current->width / 2) / PX_PER_M,
                         -(current->y + current->height / 2) / PX_PER_M};

            b2BodyId groundId =
                b2CreateBody(resources->physics->world, &groundBodyDef);
            b2Polygon groundBox = b2MakeBox(current->width / PX_PER_M / 2,
                                            current->height / PX_PER_M / 2);
            b2ShapeDef groundShapeDef = b2DefaultShapeDef();
            b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);
            break;
        }
        case OT_POLYGON:
        {
            b2BodyDef groundBodyDef = b2DefaultBodyDef();
            groundBodyDef.position =
                (b2Vec2){(current->x + current->width / 2) / PX_PER_M,
                         -(current->y + current->height / 2) / PX_PER_M};

            b2BodyId groundId =
                b2CreateBody(resources->physics->world, &groundBodyDef);

            b2Vec2 *points =
                malloc(sizeof(b2Vec2) * current->content.shape->points_len);
            for (i32 j = 0; j < current->content.shape->points_len; j++)
            {
                points[j].x = current->content.shape->points[j][0] / PX_PER_M;
                points[j].y = current->content.shape->points[j][1] / -PX_PER_M;
            }

            b2Hull groundHull =
                b2ComputeHull(points, current->content.shape->points_len);
            b2Polygon groundPolygon = b2MakePolygon(&groundHull, 0.0);
            b2ShapeDef groundShapeDef = b2DefaultShapeDef();
            b2CreatePolygonShape(groundId, &groundShapeDef, &groundPolygon);
            break;
        }
        case OT_ELLIPSE:
        {
            b2BodyDef groundBodyDef = b2DefaultBodyDef();
            groundBodyDef.position =
                (b2Vec2){(current->x + current->width / 2) / PX_PER_M,
                         -(current->y + current->width / 2) / PX_PER_M};

            b2BodyId groundId =
                b2CreateBody(resources->physics->world, &groundBodyDef);
            b2Circle groundCircle = {
                .center = {0, 0},
                .radius = current->width / PX_PER_M / 2,
            };
            b2ShapeDef groundShapeDef = b2DefaultShapeDef();
            b2CreateCircleShape(groundId, &groundShapeDef, &groundCircle);
            break;
        }
        case OT_POINT:
        {
            if (strcmp(current->name, "spawnpoint") == 0)
                *player_position =
                    (b2Vec2){current->x / PX_PER_M, -current->y / PX_PER_M};
            break;
        }
        default:
            log_warn("Unhandled object type: %d", current->obj_type);
            break;
        }
        current = current->next;
    }
}

static void handle_light_layer(tmx_layer *layer, Resources *resources)
{
    tmx_object *current = layer->content.objgr->head;
    while (current)
    {
        // color is encoded as ARGB
        tmx_property *color_prop =
            tmx_get_property(current->properties, "color");
        u8 r = color_prop->value.color >> 16;
        u8 g = color_prop->value.color >> 8;
        u8 b = color_prop->value.color;
        vec3s color = {.x = r / 255.0, .y = g / 255.0, .z = b / 255.0};

        tmx_property *intensity_prop =
            tmx_get_property(current->properties, "intensity");
        f32 intensity = intensity_prop->value.decimal;

        tmx_property *volumetric_intensity_prop =
            tmx_get_property(current->properties, "volumetric_intensity");
        f32 volumetric_intensity = volumetric_intensity_prop->value.decimal;

        tmx_property *casts_shadows_prop =
            tmx_get_property(current->properties, "casts_shadows");
        bool casts_shadows = casts_shadows_prop->value.boolean;

        switch (current->obj_type)
        {
        case OT_POLYGON:
        {

            break;
        }
        case OT_SQUARE:
        case OT_ELLIPSE:
        {
            f32 radius = current->width / 2;
            vec2s position =
                (vec2s){.x = current->x + radius, .y = current->y + radius};

            PointLight *light = malloc(sizeof(PointLight));

            light->position = position;
            light->color = color;
            light->intensity = intensity;
            light->radius = radius;
            light->volumetric_intensity = volumetric_intensity;

            light->casts_shadows = casts_shadows;
            if (casts_shadows)
            {
                light->shadowmap_entry =
                    shadowmap_add(&resources->graphics->shadowmap, position);
            }

            layer_add(&resources->graphics->lights, light);

            break;
        }
        case OT_POLYLINE:
        {
            double **points = current->content.shape->points;

            vec2s start = (vec2s){.x = points[0][0], .y = points[0][1]};
            vec2s end = (vec2s){.x = points[1][0], .y = points[1][1]};

            f32 angle = atan2(end.y - start.y, end.x - start.x);

            DirectionalLight *light = malloc(sizeof(DirectionalLight));

            light->color = color;
            light->intensity = intensity;
            light->volumetric_intensity = volumetric_intensity;

            light->casts_shadows = casts_shadows;
            if (casts_shadows)
            {
                vec2s really_far = (vec2s){.x = 1000000, .y = 1000000};
                vec2s position = glms_vec2_rotate(really_far, angle);
                position.y = -position.y;

                light->shadowmap_entry =
                    shadowmap_add(&resources->graphics->shadowmap, position);
            }

            layer_add(&resources->graphics->directional, light);

            break;
        }
        default:
            log_warn("Unhandled object type: %d", current->obj_type);
            break;
        }
        current = current->next;
    }
}

static void handle_image_layer(tmx_layer *layer, Resources *resources)
{
    tmx_image *image = layer->content.image;

    char *actual_path = tiled_image_path_to_actual(image->source);
    TextureEntry *texture_entry =
        texture_manager_load(&resources->graphics->texture_manager, actual_path,
                             &resources->graphics->wgpu);
    free(actual_path);

    Transform transform = transform_from_xyz(layer->offsetx, layer->offsety, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, transform);

    Rect rect = rect_from_size((vec2s){.x = image->width, .y = image->height});
    Rect tex_coords = RECT_UNIT_TEX_COORDS;
    Quad quad = {.rect = rect, .tex_coords = tex_coords};

    QuadEntry quad_entry =
        quad_manager_add(&resources->graphics->quad_manager, quad);

    Sprite *sprite = malloc(sizeof(Sprite));
    sprite_init(sprite, texture_entry, transform_entry, quad_entry);
    layer_add(&resources->graphics->sprite_layers.background, sprite);
}

static void handle_tile_layer(tmx_layer *layer, Resources *resources,
                              MapData *map_data)
{
    map_data->layers++;
    u32 layer_size = map_data->width * map_data->height;
    map_data->tiles =
        realloc(map_data->tiles, layer_size * map_data->layers * sizeof(i32));

    u32 start = (map_data->layers - 1) * layer_size;
    for (u32 i = 0; i < layer_size; i++)
    {
        map_data->tiles[start + i] = layer->content.gids[i] - 1;
    }

    TilemapLayer *tilemap_layer = malloc(sizeof(TilemapLayer));
    tilemap_layer->tilemap = map_data->tilemap;
    tilemap_layer->layer = map_data->layers - 1;

    layer_add(&resources->graphics->tilemap_layers.middle, tilemap_layer);
}

static void handle_map_layers(tmx_layer *head, Resources *resources,
                              b2Vec2 *player_position, MapData *map_data)
{
    tmx_layer *current = head;
    while (current)
    {
        switch (current->type)
        {
        case L_GROUP:
            handle_map_layers(current->content.group_head, resources,
                              player_position, map_data);
            break;
        case L_OBJGR:
            if (!strcmp(current->class_type, COLLISION_CLASS))
            {
                handle_collision_layer(current, resources, player_position);
            }
            else if (!strcmp(current->class_type, LIGHTS_CLASS))
            {
                handle_light_layer(current, resources);
            }
            else
            {
                log_warn("Unhandled object group class type: %s",
                         current->class_type);
            }
            break;
        case L_IMAGE:
            handle_image_layer(current, resources);
            break;
        case L_LAYER:
            handle_tile_layer(current, resources, map_data);
            break;
        case L_NONE:
            FATAL("Unrecognized layer type");
        }

        current = current->next;
    }
}

void map_scene_init(Scene **scene_data, Resources *resources, void *extra_args)
{
    MapInitArgs *args = (MapInitArgs *)extra_args;

    MapScene *map_scene = malloc(sizeof(MapScene));
    map_scene->type = Scene_Map;
    *scene_data = (Scene *)map_scene;

    map_scene->freecam = false;

    tmx_map *map = tmx_load(args->map_path);
    if (!map)
    {
        FATAL("Failed to load map %s: %s", args->map_path, tmx_strerr());
    }

    // check that we've only got one tileset!!!
    INV_PTR_ERRCHK(map->ts_head->next, "Map only has one tileset");
    // check that it's a tile atlas as well, rather than a collection of images
    PTR_ERRCHK(map->ts_head->tileset->image,
               "Tileset does not have an image source");

    MapData map_data = {
        .tiles = NULL,
        .width = map->width,
        .height = map->height,
        .layers = 0,
        .tilemap = &map_scene->tilemap,
    };
    b2Vec2 player_position = {0, 0};
    handle_map_layers(map->ly_head, resources, &player_position, &map_data);

    char *actual_path =
        tiled_image_path_to_actual(map->ts_head->tileset->image->source);
    TextureEntry *tileset_texture =
        texture_manager_load(&resources->graphics->texture_manager, actual_path,
                             &resources->graphics->wgpu);
    free(actual_path);

    Transform transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, transform);

    tilemap_init(&map_scene->tilemap, resources->graphics, tileset_texture,
                 transform_entry, map_data.width, map_data.height,
                 map_data.layers, map_data.tiles);

    free(map_data.tiles);

    tmx_map_free(map);

    player_init(&map_scene->player, player_position, resources);
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
