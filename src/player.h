#pragma once

#include "core_types.h"
#include "graphics/sprite.h"
#include "scenes/scene.h"

#define PLAYER_W 10
#define PLAYER_H 18
#define PLAYER_HW (PLAYER_W / 2.0)
#define PLAYER_HH (PLAYER_H / 2.0)

typedef struct
{
    // the player's *rendered* position
    Transform transform;
    Quad quad;

    // sprite stores quad, transform, and texture entries, so we don't need to
    // HOWEVER, we store the sprite in a layer for rendering, so we need to keep
    // it's layer entry
    Sprite sprite;
    ShadowCaster shadow_caster;

    LayerEntry layer_entry;
    LayerEntry shadow_caster_entry;

    b2BodyId body_id;
    b2ShapeId shape_id;
    b2ShapeId foot_id;

    u32 foot_contact_count;

    f32 jump_timeout;
    f32 fall_time;
    bool jumping;

    enum
    {
        Facing_Left,
        Facing_Right
    } facing;

    vec2s old_position, position;

    // inputs that have been accumulated in-between fixed timesteps, to be
    // applied at a fixed timestep
    struct
    {
        bool left, right;
        bool jump;
    } accumulated_inputs;
} Player;

void player_init(Player *player, b2Vec2 initial_pos, Resources *resources);

void player_update(Player *player, Resources *resources, bool disable_input);
void player_fixed_update(Player *player, Resources *resources);

void player_jump(Player *player);
void player_free(Player *player, Resources *resources);
