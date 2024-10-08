#pragma once

#include "core_types.h"
#include "graphics/sprite.h"
#include "scenes/scene.h"

// general player state should be kept here, like inventory and puzzle flags

typedef struct
{
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
} Player;

void player_init(Player *player, b2Vec2 initial_pos, Resources *resources);
void player_update(Player *player, Resources *resources, bool disable_input);
void player_jump(Player *player);
void player_free(Player *player, Resources *resources);
