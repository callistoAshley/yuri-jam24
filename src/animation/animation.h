#pragma once

#include "animation/definition.h"
#include "graphics/caster_manager.h"
#include "graphics/graphics.h"
#include "graphics/sprite.h"
#include "scenes/scene.h"

typedef struct
{
    const AnimationDef *def;

    f32 wait_time;
    u32 current_frame;
    bool needs_apply, finished;
} Animation;

void animation_init(Animation *animation, AnimationType type);

void animation_reset(Animation *animation);

void animation_update(Animation *animation, Resources *resources);
// i'd prefer to remove the Quad pointer arg but fetching quads is quite
// expensive
void animation_apply(Animation *animation, Graphics *graphics, Quad *quad,
                     Sprite *sprite);
void animation_apply_caster(Animation *animation, ShadowCaster *caster);

void animation_free(Animation *animation);
