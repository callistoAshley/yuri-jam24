#include "player.h"
#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "cglm/struct/vec2.h"
#include "core_types.h"
#include "graphics/caster_manager.h"
#include "physics/physics.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"
#include "utility/log.h"

// TODO account for coyote time
void foot_begin_contact_fn(b2SensorBeginTouchEvent *event, void *userdata)
{
    (void)event;
    Player *player = userdata;
    player->foot_contact_count++;
}
void foot_end_contact_fn(b2SensorEndTouchEvent *event, void *userdata)
{
    (void)event;
    Player *player = userdata;
    player->foot_contact_count--;
}

void player_init(Player *player, b2Vec2 initial_pos, Resources *resources)
{
    player->transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, player->transform);

    // player is 10x18px
    Rect rect = rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 10, .y = 18});
    Rect tex_coords =
        rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 0.5, .y = 1.0});
    player->quad = (Quad){rect, tex_coords};
    QuadEntry quad_entry =
        quad_manager_add(&resources->graphics->quad_manager, player->quad);

    TextureEntry *texture = texture_manager_load(
        &resources->graphics->texture_manager, "assets/textures/player.png",
        &resources->graphics->wgpu);

    sprite_init(&player->sprite, texture, transform_entry, quad_entry);

    player->layer_entry =
        layer_add(&resources->graphics->sprite_layers.middle, &player->sprite);

    CasterEntry *caster =
        caster_manager_load(&resources->graphics->caster_manager,
                            "assets/shadowcasters/player.shdw");

    player->shadow_caster.caster = caster;
    player->shadow_caster.cell = 0;
    player->shadow_caster.transform = transform_entry;
    player->shadow_caster.offset = (vec2s){.x = 0, .y = 0};

    player->shadow_caster_entry =
        layer_add(&resources->graphics->shadowcasters, &player->shadow_caster);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = initial_pos;
    bodyDef.fixedRotation = true;
    player->body_id = b2CreateBody(resources->physics->world, &bodyDef);

    b2Polygon dynamicBox = b2MakeBox(5.0 / PX_PER_M, 9.0 / PX_PER_M);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.0f;
    player->shape_id =
        b2CreatePolygonShape(player->body_id, &shapeDef, &dynamicBox);

    b2Vec2 foot_offset = {.x = 0.0, -9.0 / PX_PER_M};
    b2Polygon foot_sensor =
        b2MakeOffsetBox(2.0 / PX_PER_M, 1.0 / PX_PER_M, foot_offset, 0.0);

    b2ShapeDef foot_def = b2DefaultShapeDef();
    foot_def.isSensor = true;
    foot_def.density = 0.0;

    SensorUserdata *userdata = malloc(sizeof(SensorUserdata));
    userdata->begin = foot_begin_contact_fn;
    userdata->end = foot_end_contact_fn;
    userdata->userdata = player;
    foot_def.userData = userdata;

    player->foot_id =
        b2CreatePolygonShape(player->body_id, &foot_def, &foot_sensor);
    player->foot_contact_count = 0;

    player->jump_timeout = 0;
    player->fall_time = 0;
    player->jumping = false;

    player->facing = Facing_Left;
}

#define WALK_SPEED_MPS 4
#define WALK_SPEED_CAP 8
#define WALK_SPEED_PXPS M_TO_PX(WALK_SPEED_MPS)
void player_update(Player *player, Resources *resources, bool disable_input)
{
    bool left_down =
        input_is_down(resources->input, Button_Left) && !disable_input;
    bool right_down =
        input_is_down(resources->input, Button_Right) && !disable_input;

    if (left_down && !right_down)
        player->facing = Facing_Left;
    if (right_down && !left_down)
        player->facing = Facing_Right;

    f32 walk_speed = WALK_SPEED_PXPS * resources->input->delta_seconds;
    f32 current_speed = b2Body_GetLinearVelocity(player->body_id).x;
    if (left_down && current_speed > -WALK_SPEED_CAP)
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){-walk_speed, 0}, true);
    else if (right_down && current_speed < WALK_SPEED_CAP)
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){walk_speed, 0}, true);
    else if (current_speed) // not moving, decelerate
        b2Body_ApplyLinearImpulseToCenter(
            player->body_id, (b2Vec2){-current_speed / 4, 0}, true);

    if (player->jump_timeout > 0)
        player->jump_timeout -= resources->input->delta_seconds;

    if (player->foot_contact_count > 0 && player->jump_timeout <= 0)
    {
        // we're on the ground
        player->fall_time = 0;
        player->jumping = false;
        if (input_is_down(resources->input, Button_Up) && !disable_input)
        {
            player_jump(player);
        }
    }
    else if (!player->foot_contact_count && player->jump_timeout <= 0 &&
             !player->jumping)
    {
        player->fall_time += resources->input->delta_seconds;
        if (player->fall_time < 0.3f &&
            input_is_down(resources->input, Button_Up) && !disable_input)
        {
            // coyote jump
            player_jump(player);
        }
    }

    b2Vec2 body_position = b2Body_GetPosition(player->body_id);
    // box2d has a different coordinate system than us
    // +y is up for box2d, down for us
    // so we need to negate the y component
    player->transform.position.x = body_position.x * PX_PER_M - 5;
    player->transform.position.y = -body_position.y * PX_PER_M - 9;

    if (player->facing == Facing_Left)
    {
        player->quad.tex_coords.min.x = 0.5;
        player->quad.tex_coords.max.x = 1.0;
        player->shadow_caster.cell = 1;
    }
    else
    {
        player->quad.tex_coords.min.x = 0.0;
        player->quad.tex_coords.max.x = 0.5;
        player->shadow_caster.cell = 0;
    }

    quad_manager_update(&resources->graphics->quad_manager, player->sprite.quad,
                        player->quad);

    transform_manager_update(&resources->graphics->transform_manager,
                             player->sprite.transform, player->transform);
}

void player_jump(Player *player)
{
    b2Body_ApplyLinearImpulseToCenter(player->body_id, (b2Vec2){0.0, 22.5},
                                      true);
    player->jump_timeout = 0.1;
    player->jumping = true;
}

void player_free(Player *player, Resources *resources)
{
    SensorUserdata *userdata = b2Shape_GetUserData(player->foot_id);
    free(userdata);

    b2DestroyBody(player->body_id);

    // we only need to remove the sprite from the layer-
    // sprite_free will handle the rest
    layer_remove(&resources->graphics->sprite_layers.middle,
                 player->layer_entry);
    sprite_free(&player->sprite, resources->graphics);
}
