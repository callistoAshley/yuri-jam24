#include "rigidbody.h"
#include "box2d/box2d.h"
#include "core_types.h"
#include "scenes/map.h"
#include "tmx.h"
#include "utility/common_defines.h"
#include "utility/log.h"

void *rigidbody_char_init(Resources *resources, struct MapScene *map_scene,
                          CharacterInitArgs *args)
{
    (void)map_scene;

    RigidBodyCharState *state = malloc(sizeof(RigidBodyCharState));
    state->rect = args->rect;
    memset(state->sprite_name, 0, sizeof(state->sprite_name));

    vec2s half_size = glms_vec2_mul(rect_size(args->rect), VEC2_SPLAT(0.5));

    vec2s rotated_center_pos = glms_vec2_rotate(half_size, args->rotation);
    vec2s initial_pos = glms_vec2_add(args->rect.min, rotated_center_pos);

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position.x = PX_TO_M(initial_pos.x);
    body_def.position.y = PX_TO_M(-initial_pos.y);
    body_def.rotation = b2MakeRot(args->rotation);
    state->body_id = b2CreateBody(resources->physics->world, &body_def);

    b2ShapeDef shape_def = b2DefaultShapeDef();

    switch (args->object_type)
    {
    case OT_ELLIPSE:
    {
        if (half_size.x != half_size.y)
            log_warn("Elliptical rigid bodies are unsupported");

        f32 radius = PX_TO_M(half_size.x);
        b2Circle circle = {
            .radius = radius,
        };
        b2CreateCircleShape(state->body_id, &shape_def, &circle);
        break;
    }
    case OT_SQUARE:
    {
        b2Polygon box = b2MakeBox(PX_TO_M(half_size.x), PX_TO_M(half_size.y));
        b2CreatePolygonShape(state->body_id, &shape_def, &box);
        break;
    }
    default:
    {
        log_warn("Unhandled object type %d", args->object_type);
        b2Polygon box = b2MakeBox(PX_TO_M(half_size.x), PX_TO_M(half_size.y));
        b2CreatePolygonShape(state->body_id, &shape_def, &box);
        break;
    }
    }

    state->b2d_position = b2Body_GetTransform(state->body_id);
    state->old_b2d_position = state->b2d_position;

    if (hashmap_get(args->metadata, "sprite"))
    {
        state->transform =
            transform_from_xyz(args->rect.min.x, args->rect.min.y, 0);
        TransformEntry transform = transform_manager_add(
            &resources->graphics->transform_manager, state->transform);

        strncpy(state->sprite_name, hashmap_get(args->metadata, "sprite"),
                sizeof(state->sprite_name));
        TextureEntry *texture = texture_manager_load(
            &resources->graphics->texture_manager, state->sprite_name,
            &resources->graphics->wgpu);

        Rect quad_rect = rect_from_center_radius(GLMS_VEC2_ZERO, half_size);
        state->quad = quad_init(quad_rect, RECT_UNIT_TEX_COORDS);

        sprite_init(
            &state->sprite, texture, transform,
            quad_manager_add(&resources->graphics->quad_manager, state->quad));
        state->layer_entry = layer_add(
            &resources->graphics->sprite_layers.middle, &state->sprite);
    }

    return state;
}

void rigidbody_char_fixed_update(void *self, Resources *resources,
                                 MapScene *map_scene)
{
    (void)map_scene;
    (void)resources;
    RigidBodyCharState *state = self;

    state->old_b2d_position = state->b2d_position;
    state->b2d_position = b2Body_GetTransform(state->body_id);
}

void rigidbody_char_update(void *self, Resources *resources,
                           MapScene *map_scene)
{
    (void)map_scene;
    RigidBodyCharState *state = self;

    vec2s old_position;
    old_position.x = state->old_b2d_position.p.x;
    old_position.y = state->old_b2d_position.p.y;

    vec2s position;
    position.x = state->b2d_position.p.x;
    position.y = state->b2d_position.p.y;

    f32 lerp_coefficient = time_fixed_overstep_fraction(resources->time.fixed);
    vec2s interpolated_position =
        glms_vec2_lerp(old_position, position, lerp_coefficient);

    f32 angle = b2Rot_GetAngle(state->b2d_position.q);

    state->transform.position.x = M_TO_PX(interpolated_position.x);
    state->transform.position.y = M_TO_PX(-interpolated_position.y);

    // need to negate the angle because of box2d's coordinate system
    state->transform.rotation = glms_euler_xyz_quat((vec3s){.z = -angle});

    transform_manager_update(&resources->graphics->transform_manager,
                             state->sprite.transform, state->transform);
}

void rigidbody_char_free(void *self, Resources *resources, MapScene *map_scene)
{
    (void)map_scene;
    RigidBodyCharState *state = self;

    b2DestroyBody(state->body_id);

    if (state->sprite.texture)
    {
        sprite_free(&state->sprite, resources->graphics);
        layer_remove(&resources->graphics->sprite_layers.middle,
                     state->layer_entry);
    }
    free(state);
}
