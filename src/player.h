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
    LayerEntry layer_entry;

    b2BodyId body_id;
} Player;

void player_init(Player *player, Resources *resources);
void player_update(Player *player, Resources *resources);
void player_free(Player *player, Resources *resources);
