#include "map_loader.h"
#include "utility/log.h"

#define COLLISION_CLASS "collision"
#define LIGHTS_CLASS "lights"

static char *tiled_image_path_to_actual(char *path)
{
    const char *prefix = "assets/textures/";
    char *new_path = malloc(strlen(prefix) + strlen(path) + 1);
    strcpy(new_path, prefix);
    strcat(new_path, path);
    return new_path;
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
                b2CreateBody(resources->physics->world, &groundBodyDef);
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
                b2CreateBody(resources->physics->world, &groundBodyDef);
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

        tmx_property *casts_shadows_prop =
            tmx_get_property(current->properties, "casts_shadows");
        renderable.data.light->casts_shadows =
            casts_shadows_prop->value.boolean;

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

            if (renderable.data.light->casts_shadows)
            {
                renderable.data.light->shadowmap_entry =
                    shadowmap_add(&resources->graphics->shadowmap, position);
            }

            break;
        }
        case OT_POLYLINE:
        {
            renderable.data.light->type = Light_Direct;

            double **points = current->content.shape->points;

            vec2s start = (vec2s){.x = points[0][0], .y = points[0][1]};
            vec2s end = (vec2s){.x = points[1][0], .y = points[1][1]};

            f32 angle = atan2(end.y - start.y, end.x - start.x);

            if (renderable.data.light->casts_shadows)
            {
                vec2s really_far = (vec2s){.x = 1000000, .y = 1000000};
                vec2s position = glms_vec2_rotate(really_far, angle);
                position.y = -position.y;

                renderable.data.light->shadowmap_entry =
                    shadowmap_add(&resources->graphics->shadowmap, position);
            }

            break;
        }
        default:
            log_warn("Unhandled object type: %d", current->obj_type);
            break;
        }

        renderable.entry =
            layer_add(&resources->graphics->lights, renderable.data.light);
        vec_push(load->renderables, &renderable);

        current = current->next;
    }
}

void handle_shadow_layer(tmx_layer *layer, Resources *resources,
                         MapLoadArgs *load)
{
    vec points;
    vec_init(&points, sizeof(vec2s));

    tmx_object *current = layer->content.objgr->head;
    while (current)
    {
        switch (current->obj_type)
        {
        case OT_SQUARE:
        {
            // we have to push each point twice, because the shadow system works
            // off line segments
            vec2s first = (vec2s){.x = current->x, .y = current->y};
            vec_push(&points, &first);

            vec2s point =
                (vec2s){.x = current->x + current->width, .y = current->y};
            vec_push(&points, &point);
            vec_push(&points, &point);

            point = (vec2s){.x = current->x + current->width,
                            .y = current->y + current->height};
            vec_push(&points, &point);
            vec_push(&points, &point);

            point = (vec2s){.x = current->x, .y = current->y + current->height};
            vec_push(&points, &point);
            vec_push(&points, &point);

            vec_push(&points, &first);

            break;
        }
        case OT_POLYGON:
        {
            double **obj_points = current->content.shape->points;
            i32 obj_point_len = current->content.shape->points_len;
            vec2s obj_position = (vec2s){.x = current->x, .y = current->y};

            // we have to have a special case for the last point, so only
            // iterate to the second last point
            for (i32 i = 0; i < obj_point_len - 1; i++)
            {
                vec2s point =
                    (vec2s){.x = obj_points[i][0], .y = obj_points[i][1]};
                point = glms_vec2_add(point, obj_position);
                vec_push(&points, &point);

                // push i+1
                point = (vec2s){.x = obj_points[i + 1][0],
                                .y = obj_points[i + 1][1]};
                point = glms_vec2_add(point, obj_position);
                vec_push(&points, &point);
            }

            // push the last point
            vec2s point = (vec2s){.x = obj_points[obj_point_len - 1][0],
                                  .y = obj_points[obj_point_len - 1][1]};
            point = glms_vec2_add(point, obj_position);
            vec_push(&points, &point);

            // ... then push the first point again
            point = (vec2s){.x = obj_points[0][0], .y = obj_points[0][1]};
            point = glms_vec2_add(point, obj_position);
            vec_push(&points, &point);

            break;
        }

        default:
            log_warn("Unhandled object type: %d", current->obj_type);
            break;
        }

        current = current->next;
    }

    // now that we're done, we can register the shadow caster
    Cell cell = {
        .points = (vec2s *)points.data,
        .point_count = points.len,
    };
    CasterEntry *caster_entry =
        caster_manager_register(&resources->graphics->caster_manager, &cell, 1);

    // now we can add the shadow caster to the scene
    Transform transform = transform_from_xyz(layer->offsetx, layer->offsety, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, transform);

    MapRenderable renderable;
    renderable.type = Map_Caster;

    renderable.data.caster = malloc(sizeof(ShadowCaster));
    renderable.data.caster->transform = transform_entry;
    renderable.data.caster->caster = caster_entry;
    renderable.data.caster->offset = (vec2s){.x = 0, .y = 0};
    renderable.data.caster->cell = 0;

    renderable.entry =
        layer_add(&resources->graphics->shadowcasters, renderable.data.caster);

    vec_push(load->renderables, &renderable);
}

void handle_image_layer(tmx_layer *layer, Resources *resources,
                        MapLoadArgs *load)
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

    MapRenderable renderable;
    renderable.type = Map_Sprite;
    renderable.data.sprite.ptr = malloc(sizeof(Sprite));
    sprite_init(renderable.data.sprite.ptr, texture_entry, transform_entry,
                quad_entry);
    renderable.entry = layer_add(&resources->graphics->sprite_layers.background,
                                 renderable.data.sprite.ptr);
    renderable.data.sprite.layer = Layer_Back;

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

    renderable.entry = layer_add(&resources->graphics->tilemap_layers.middle,
                                 renderable.data.tile.ptr);
    renderable.data.tile.layer = Layer_Middle;

    vec_push(load->renderables, &renderable);
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
            else if (!strcmp(current->class_type, "shadows"))
            {
                handle_shadow_layer(current, resources, load);
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
