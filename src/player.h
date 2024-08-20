#pragma once

#include "cglm/quat.h"
#include "core_types.h"
#include "graphics/graphics.h"
#include "graphics/quad_manager.h"
#include "graphics/sprite.h"
#include "input/input.h"
#include "physics/physics.h"

// general player state should be kept here, like inventory and puzzle flags

typedef struct
{
    Camera camera;
    Transform transform;
    Quad quad;

    // sprite stores quad, transform, and texture entries, so we don't need to
    // HOWEVER, we store the sprite in a layer for rendering, so we need to keep
    // it's layer entry
    Sprite sprite;
    LayerEntry layer_entry;

    b2BodyId body_id;
} Player;

void player_init(Player *player, Graphics *graphics, Physics *physics);
void player_update(Player *player, Graphics *graphics, Input *input);
void player_free(Player *player, Graphics *graphics);
