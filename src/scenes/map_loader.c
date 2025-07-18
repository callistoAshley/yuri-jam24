#include "map_loader.h"
#include "cglm/types-struct.h"
#include "characters/character.h"
#include "graphics/graphics.h"
#include "resources.h"
#include "utility/common_defines.h"
#include "utility/log.h"

#define COLLISION_CLASS "collision"
#define LIGHTS_CLASS "lights"
#define CHARACTERS_CLASS "characters"

static char *tiled_image_path_to_actual(char *path)
{
    const char *prefix = "assets/textures/";
    char *new_path = malloc(strlen(prefix) + strlen(path) + 1);
    strcpy(new_path, prefix);
    strcat(new_path, path);
    return new_path;
}

static void layer_for(i32 layer, StandardLayers *layers, Layer **target,
                      MapLayer *target_layer)
{
    switch (layer)
    {
    case 1:
        *target = &layers->background;
        *target_layer = Layer_Back;
        break;
    case 2:
        *target = &layers->middle;
        *target_layer = Layer_Middle;
        break;
    case 3:
        *target = &layers->foreground;
        *target_layer = Layer_Front;
        break;
    default:
        log_warn("unrecognized layer %d", layer);
        *target = &layers->background;
        *target_layer = Layer_Back;
        break;
    }
}

void handle_collision_layer(tmx_layer *layer, Resources *resources,
                            MapLoadArgs *load)
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
                b2CreateBody(resources->physics.world, &groundBodyDef);
            b2Polygon groundBox = b2MakeBox(current->width / PX_PER_M / 2,
                                            current->height / PX_PER_M / 2);
            b2ShapeDef groundShapeDef = b2DefaultShapeDef();
            b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

            vec_push(load->colliders, &groundId);
            break;
        }
        case OT_POLYGON:
        {
            b2BodyDef groundBodyDef = b2DefaultBodyDef();
            groundBodyDef.position =
                (b2Vec2){(current->x + current->width / 2) / PX_PER_M,
                         -(current->y + current->height / 2) / PX_PER_M};
            b2BodyId groundId =
                b2CreateBody(resources->physics.world, &groundBodyDef);

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

            vec_push(load->colliders, &groundId);
            break;
        }
        case OT_ELLIPSE:
        {
            b2BodyDef groundBodyDef = b2DefaultBodyDef();
            groundBodyDef.position =
                (b2Vec2){(current->x + current->width / 2) / PX_PER_M,
                         -(current->y + current->width / 2) / PX_PER_M};

            b2BodyId groundId =
                b2CreateBody(resources->physics.world, &groundBodyDef);
            b2Circle groundCircle = {
                .center = {0, 0},
                .radius = current->width / PX_PER_M / 2,
            };
            b2ShapeDef groundShapeDef = b2DefaultShapeDef();
            b2CreateCircleShape(groundId, &groundShapeDef, &groundCircle);

            vec_push(load->colliders, &groundId);
            break;
        }
        case OT_POINT:
        {
            if (strcmp(current->name, "spawnpoint") == 0)
                *load->player_position =
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

void handle_light_layer(tmx_layer *layer, Resources *resources,
                        MapLoadArgs *load)
{
    tmx_object *current = layer->content.objgr->head;
    while (current)
    {
        MapRenderable renderable;
        renderable.type = Map_Light;
        renderable.data.light = malloc(sizeof(Light));

        // color is encoded as ARGB
        tmx_property *color_prop =
            tmx_get_property(current->properties, "color");
        u8 r = color_prop->value.color >> 16;
        u8 g = color_prop->value.color >> 8;
        u8 b = color_prop->value.color;
        renderable.data.light->color =
            (vec3s){.x = r / 255.0, .y = g / 255.0, .z = b / 255.0};

        tmx_property *intensity_prop =
            tmx_get_property(current->properties, "intensity");
        renderable.data.light->intensity = intensity_prop->value.decimal;

        tmx_property *volumetric_intensity_prop =
            tmx_get_property(current->properties, "volumetric_intensity");
        renderable.data.light->volumetric_intensity =
            volumetric_intensity_prop->value.decimal;

        switch (current->obj_type)
        {
        case OT_POLYGON:
        {

            break;
        }
        case OT_SQUARE:
        case OT_ELLIPSE:
        {
            renderable.data.light->type = Light_Point;

            f32 radius = current->width / 2;
            vec2s position =
                (vec2s){.x = current->x + radius, .y = current->y + radius};

            renderable.data.light->data.point.position = position;
            renderable.data.light->data.point.radius = radius;

            break;
        }
        case OT_POLYLINE:
        {
            renderable.data.light->type = Light_Direct;

            break;
        }
        default:
            log_warn("Unhandled object type: %d", current->obj_type);
            break;
        }

        renderable.entry =
            layer_add(&resources->graphics.lights, renderable.data.light);
        vec_push(load->renderables, &renderable);

        current = current->next;
    }
}

void handle_image_layer(tmx_layer *layer, Resources *resources,
                        MapLoadArgs *load)
{
    tmx_image *image = layer->content.image;

    char *actual_path = tiled_image_path_to_actual(image->source);
    TextureEntry *texture_entry =
        texture_manager_load(&resources->graphics.texture_manager, actual_path,
                             &resources->graphics.wgpu);
    free(actual_path);

#define REPEAT_LAYER_DIM_LEN 100000
#define REPEAT_LAYER_DIM_OFFSET (-REPEAT_LAYER_DIM_LEN / 2)

    Rect rect;
    Rect tex_coords;
    i32 x;
    i32 y;

    if (layer->repeatx && layer->repeaty)
    {
        rect = rect_from_size(
            (vec2s){.x = REPEAT_LAYER_DIM_LEN, .y = REPEAT_LAYER_DIM_LEN});
        tex_coords = rect_from_size((vec2s){
            .x = (f32)REPEAT_LAYER_DIM_LEN / image->width,
            .y = (f32)REPEAT_LAYER_DIM_LEN / image->height,
        });
        x = REPEAT_LAYER_DIM_OFFSET - layer->offsetx;
        y = REPEAT_LAYER_DIM_OFFSET - layer->offsety;
    }
    else if (layer->repeatx)
    {
        rect = rect_from_size(
            (vec2s){.x = REPEAT_LAYER_DIM_LEN, .y = image->height});
        tex_coords = rect_from_size(
            (vec2s){.x = (f32)REPEAT_LAYER_DIM_LEN / image->width, .y = 1.0});
        x = REPEAT_LAYER_DIM_OFFSET - layer->offsetx;
        y = layer->offsety;
    }
    else if (layer->repeaty)
    {
        rect = rect_from_size(
            (vec2s){.x = image->width, .y = REPEAT_LAYER_DIM_LEN});
        tex_coords = rect_from_size(
            (vec2s){.x = 1.0, .y = (f32)REPEAT_LAYER_DIM_LEN / image->height});
        x = layer->offsetx;
        y = REPEAT_LAYER_DIM_OFFSET - layer->offsety;
    }
    else
    {
        rect = rect_from_size((vec2s){.x = image->width, .y = image->height});
        tex_coords = RECT_UNIT_TEX_COORDS;
        x = layer->offsetx;
        y = layer->offsety;
    };

    Transform transform = transform_from_xyz(x, y, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics.transform_manager, transform);

    Quad quad = {.rect = rect, .tex_coords = tex_coords};

    QuadEntry quad_entry =
        quad_manager_add(&resources->graphics.quad_manager, quad);

    MapRenderable renderable;
    renderable.type = Map_Sprite;
    renderable.data.sprite.ptr = malloc(sizeof(Sprite));
    sprite_init(renderable.data.sprite.ptr, texture_entry, transform_entry,
                quad_entry);
    renderable.data.sprite.ptr->parallax_factor =
        (vec2s){.x = layer->parallaxx, .y = layer->parallaxy};

    Layer *target = &resources->graphics.sprite_layers.middle;
    MapLayer target_enum = Layer_Middle;
    tmx_property *prop = tmx_get_property(layer->properties, "layer");
    if (prop)
    {
        layer_for(prop->value.integer, &resources->graphics.sprite_layers,
                  &target, &target_enum);
    }

    renderable.entry = layer_add(target, renderable.data.sprite.ptr);
    renderable.data.sprite.layer = target_enum;

    vec_push(load->renderables, &renderable);
}

void handle_tile_layer(tmx_layer *layer, Resources *resources,
                       MapLoadArgs *load)
{
    load->layers++;
    u32 layer_size = load->width * load->height;
    load->tiles = realloc(load->tiles, layer_size * load->layers * sizeof(i32));

    u32 start = (load->layers - 1) * layer_size;
    for (u32 i = 0; i < layer_size; i++)
    {
        load->tiles[start + i] = layer->content.gids[i] - 1;
    }

    MapRenderable renderable;
    renderable.type = Map_TileLayer;

    renderable.data.tile.ptr = malloc(sizeof(TilemapLayer));
    renderable.data.tile.ptr->tilemap = load->tilemap;
    renderable.data.tile.ptr->layer = load->layers - 1;
    renderable.data.tile.ptr->parallax_factor =
        (vec2s){.x = layer->parallaxx, .y = layer->parallaxy};

    Layer *target = &resources->graphics.tilemap_layers.middle;
    MapLayer target_enum = Layer_Middle;
    tmx_property *prop = tmx_get_property(layer->properties, "layer");
    if (prop)
    {
        layer_for(prop->value.integer, &resources->graphics.tilemap_layers,
                  &target, &target_enum);
    }

    renderable.entry = layer_add(target, renderable.data.tile.ptr);
    renderable.data.tile.layer = target_enum;

    vec_push(load->renderables, &renderable);
}

static void prop_foreach_func(tmx_property *prop, void *ud)
{
    if (prop->type != PT_STRING)
        return;
    char key[256];
    strncpy(key, prop->name, sizeof(key) - 1);
    char value[256];
    strncpy(value, prop->value.string, sizeof(value) - 1);
    hashmap_insert(ud, key, value);
}

void handle_character_layer(tmx_layer *layer, Resources *resources,
                            MapLoadArgs *load)
{
    (void)resources;
    tmx_object *current = layer->content.objgr->head;
    while (current)
    {
        // If there was no specified type, the character uses the "basic"
        // interface
        MapCharacterObj obj;
        obj.rect = rect_from_min_size(
            (vec2s){.x = current->x, .y = current->y},
            (vec2s){.x = current->width, .y = current->height});
        obj.rotation = TO_RAD(current->rotation);
        obj.object_type = current->obj_type;

        obj.interface = CHARACTERS[Char_Basic];
        if (current->type)
        {
            CharacterType type;
            for (type = 0; type < Char_Max; type++)
            {
                CharacterInterface interface = CHARACTERS[type];
                if (!strcmp(current->type, interface.name))
                {
                    obj.interface = interface;
                    break;
                }
            }
            if (type == Char_Max)
            {
                log_warn("Unhandled character type %s", current->type);
            }
        }

        hashmap_init(&obj.properties, fnv_cstr_hash_function,
                     strlen_eq_function, 256, 256);
        tmx_property_foreach(current->properties, prop_foreach_func,
                             &obj.properties);

        vec_push(load->characters, &obj);

        current = current->next;
    }
}

void handle_map_layers(tmx_layer *head, Resources *resources, MapLoadArgs *load)
{
    tmx_layer *current = head;
    while (current)
    {
        switch (current->type)
        {
        case L_GROUP:
            handle_map_layers(current->content.group_head, resources, load);
            break;
        case L_OBJGR:
            if (!strcmp(current->class_type, COLLISION_CLASS))
            {
                handle_collision_layer(current, resources, load);
            }
            else if (!strcmp(current->class_type, LIGHTS_CLASS))
            {
                handle_light_layer(current, resources, load);
            }
            else if (!strcmp(current->class_type, CHARACTERS_CLASS))
            {
                handle_character_layer(current, resources, load);
            }
            else
            {
                log_warn("Unhandled object group class type: %s",
                         current->class_type);
            }
            break;
        case L_IMAGE:
            handle_image_layer(current, resources, load);
            break;
        case L_LAYER:
            handle_tile_layer(current, resources, load);
            break;
        case L_NONE:
            FATAL("Unrecognized layer type");
        }

        current = current->next;
    }
}
