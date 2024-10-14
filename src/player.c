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

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = initial_pos;
    bodyDef.fixedRotation = true;
    player->body_id = b2CreateBody(resources->physics->world, &bodyDef);

    f32 radius = PX_TO_M(PLAYER_HW);
    b2Capsule capsule = {
        .center1 = (b2Vec2){0, radius},
        // no idea why we need radius * 3.0. it just works
        .center2 = (b2Vec2){0, (radius * 3.0) - PX_TO_M((f32)PLAYER_H)},
        .radius = radius};
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.0f;
    player->shape_id =
        b2CreateCapsuleShape(player->body_id, &shapeDef, &capsule);

    b2Vec2 foot_offset = {.x = 0.0, PX_TO_M(-8.0)};
    b2Polygon foot_sensor =
        b2MakeOffsetBox(PX_TO_M(3.0), PX_TO_M(0.5), foot_offset, 0.0);

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

    player_update(player, resources, true);
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
    b2Vec2 current_speed = b2Body_GetLinearVelocity(player->body_id);
    if (left_down && current_speed.x > -WALK_SPEED_CAP)
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){-walk_speed, 0}, true);
    else if (right_down && current_speed.x < WALK_SPEED_CAP)
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){walk_speed, 0}, true);
    else if (current_speed.x) // not moving, decelerate
        b2Body_ApplyLinearImpulseToCenter(
            player->body_id, (b2Vec2){-current_speed.x / 2, 0}, true);

    if (player->jump_timeout > 0)
        player->jump_timeout -= resources->input->delta_seconds;

    b2Vec2 body_position = b2Body_GetPosition(player->body_id);

    if (player->fall_time <= 0.01 && player->foot_contact_count == 0)
    {
        player->falling_from = body_position;
    }

    if (player->fall_time > 0.10 && player->foot_contact_count > 0)
    {
        f32 fall_distance =
            (player->falling_from.y - body_position.y) * PX_PER_M;
        if (fall_distance > 40.0)
        {
            FMOD_STUDIO_EVENTDESCRIPTION *desc;
            FMOD_Studio_System_GetEvent(resources->audio->system,
                                        "event:/sfx/player_land", &desc);

            FMOD_STUDIO_EVENTINSTANCE *inst;
            FMOD_Studio_EventDescription_CreateInstance(desc, &inst);
            FMOD_Studio_EventInstance_SetParameterByName(inst, "fall_distance",
                                                         fall_distance, false);
            FMOD_Studio_EventInstance_Start(inst);
            FMOD_Studio_EventInstance_Release(inst);
        }
    }

    if (player->foot_contact_count == 0)
    {
        player->fall_time += resources->input->delta_seconds;
    }

    if (player->foot_contact_count > 0 && player->jump_timeout <= 0)
    {
        // we're on the ground
        player->fall_time = 0;
        player->jumping = false;
        if (input_is_down(resources->input, Button_Jump) && !disable_input)
        {
            player_jump(player);
        }
    }
    else if (!player->foot_contact_count && player->jump_timeout <= 0 &&
             !player->jumping)
    {

        if (player->fall_time < 0.3f &&
            input_is_down(resources->input, Button_Jump) && !disable_input)
        {
            // coyote jump
            player_jump(player);
        }
    }

    // box2d has a different coordinate system than us
    // +y is up for box2d, down for us
    // so we need to negate the y component
    player->transform.position.x = M_TO_PX(body_position.x) - PLAYER_HW;
    player->transform.position.y = M_TO_PX(-body_position.y) - PLAYER_HH;

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

    // FMOD has the same coordinate system as box2d (+y is up, +x is right)
    FMOD_3D_ATTRIBUTES player_attrs = {
        .position = {body_position.x, body_position.y, 0.0},
        .velocity = {current_speed.x, current_speed.y, 0.0},
        .up = {0.0, 1.0, 0.0},
        .forward = {0.0, 0.0, 1.0}};

    FMOD_RESULT result;
    result = FMOD_Studio_System_SetListenerAttributes(
        resources->audio->system, 0, &player_attrs, &player_attrs.position);
    FMOD_ERRCHK(result, "failed to set listener position");
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

    layer_remove(&resources->graphics->shadowcasters,
                 player->shadow_caster_entry);
}
