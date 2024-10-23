#include "player.h"
#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "cglm/struct/vec2.h"
#include "core_types.h"
#include "fmod_studio.h"
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
    Rect rect = rect_from_min_size(GLMS_VEC2_ZERO,
                                   (vec2s){.x = PLAYER_W, .y = PLAYER_H});
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
    player->shadow_caster.radius = 45.0;

    player->shadow_caster_entry =
        layer_add(&resources->graphics->shadowcasters, &player->shadow_caster);

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = initial_pos;
    body_def.fixedRotation = true;
    player->body_id = b2CreateBody(resources->physics->world, &body_def);

    b2Polygon player_box = b2MakeBox(PX_TO_M(PLAYER_HW), PX_TO_M(PLAYER_HH));
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.density = 1.0f;
    shape_def.friction = 0.0f;
    b2CreatePolygonShape(player->body_id, &shape_def, &player_box);

    b2Vec2 foot_offset = {.x = 0.0, PX_TO_M(-9.0)};
    b2Polygon foot_sensor =
        b2MakeOffsetBox(PX_TO_M(4.5), PX_TO_M(0.5), foot_offset, 0.0);

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

    player->facing = Facing_Left;

    player->accumulated_inputs.left = false;
    player->accumulated_inputs.right = false;
    player->accumulated_inputs.jump = false;

    b2Vec2 position = b2Body_GetPosition(player->body_id);
    player->position.x = position.x;
    player->position.y = position.y;

    player_fixed_update(player, resources);
    player_update(player, resources, true);
}

#define WALK_SPEED_MPS 4
#define WALK_SPEED_CAP 8
#define WALK_SPEED_PXPS M_TO_PX(WALK_SPEED_MPS)

void player_fixed_update(Player *player, Resources *resources)
{
    f32 delta = duration_as_secs(resources->time.current.delta);
    f32 walk_speed = WALK_SPEED_PXPS * delta;

    // in air, so decrease walk speed
    if (player->foot_contact_count == 0)
    {
        walk_speed *= 0.5;
    }

    b2Vec2 current_speed = b2Body_GetLinearVelocity(player->body_id);
    b2Vec2 body_position = b2Body_GetPosition(player->body_id);

    player->old_position = player->position;
    player->position.x = body_position.x;
    player->position.y = body_position.y;

    if (player->accumulated_inputs.left && current_speed.x > -WALK_SPEED_CAP)
    {
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){-walk_speed, 0}, true);
    }
    else if (player->accumulated_inputs.right &&
             current_speed.x < WALK_SPEED_CAP)
    {
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){walk_speed, 0}, true);
    }
    // not moving, decelerate
    else if (current_speed.x)
    {
        b2Body_ApplyLinearImpulseToCenter(
            player->body_id, (b2Vec2){-current_speed.x / 2, 0}, true);
    }

    if (player->jump_timeout > 0)
    {
        player->jump_timeout -= delta;
    }

    if (player->foot_contact_count == 0)
    {
        player->fall_time += delta;
    }

    if (player->foot_contact_count > 0 && player->jump_timeout <= 0)
    {
        // we're on the ground
        player->fall_time = 0;
        player->jumping = false;
        if (player->accumulated_inputs.jump)
        {
            player_jump(player);
        }
    }
    else if (!player->foot_contact_count && player->jump_timeout <= 0 &&
             !player->jumping)
    {
        if (player->fall_time < 0.3f && player->accumulated_inputs.jump)
        {
            // coyote jump
            player_jump(player);
        }
    }
}

void player_update(Player *player, Resources *resources, bool disable_input)
{
    bool left_down = input_is_down(resources->input, Button_Left);
    bool right_down = input_is_down(resources->input, Button_Right);

    bool left_pressed = input_is_pressed(resources->input, Button_Left);
    bool right_pressed = input_is_pressed(resources->input, Button_Right);
    bool jump_pressed = input_is_pressed(resources->input, Button_Jump);

    // if a button isn't being held, turn off the input
    if (!left_down)
        player->accumulated_inputs.left = false;
    if (!right_down)
        player->accumulated_inputs.right = false;

    // only update what directional buttons are down if only one is pressed
    if (left_down && !right_down)
        player->accumulated_inputs.left = true;
    if (right_down && !left_down)
        player->accumulated_inputs.right = true;

    // ..but if a specific button was just pressed, update the inputs instantly
    // this is so if the player was holding left, then started holding right,
    // the player will move right
    if (left_pressed)
    {
        player->accumulated_inputs.left = true;
        player->accumulated_inputs.right = false;
    }
    if (right_pressed)
    {
        player->accumulated_inputs.right = true;
        player->accumulated_inputs.left = false;
    }

    // if the jump hasn't been pressed yet, update the accumulated input
    if (!player->accumulated_inputs.jump)
    {
        player->accumulated_inputs.jump = jump_pressed;
    }

    // we shouldn't be accepting input, so force the accumulated inputs to be
    // off
    if (disable_input)
    {
        player->accumulated_inputs.left = false;
        player->accumulated_inputs.right = false;
        player->accumulated_inputs.jump = false;
    }

    if (player->accumulated_inputs.left)
    {
        player->facing = Facing_Left;
    }
    if (player->accumulated_inputs.right)
    {
        player->facing = Facing_Right;
    }

    f32 lerp_coefficient = time_fixed_overstep_fraction(resources->time.fixed);
    vec2s interpolated_position = glms_vec2_lerp(
        player->old_position, player->position, lerp_coefficient);

    // box2d has a different coordinate system than us
    // +y is up for box2d, down for us
    // so we need to negate the y component
    player->transform.position.x = M_TO_PX(interpolated_position.x) - PLAYER_HW;
    player->transform.position.y =
        M_TO_PX(-interpolated_position.y) - PLAYER_HH;

    transform_manager_update(&resources->graphics->transform_manager,
                             player->sprite.transform, player->transform);

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
}

void player_jump(Player *player)
{
    b2Body_ApplyLinearImpulseToCenter(player->body_id, (b2Vec2){0.0, 22.5},
                                      true);
    player->jump_timeout = 0.1;
    player->jumping = true;
    player->accumulated_inputs.jump = false;
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

    layer_remove(&resources->graphics->shadowcasters,
                 player->shadow_caster_entry);
}
