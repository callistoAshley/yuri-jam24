#pragma once

#include "graphics/graphics.h"
#include "physics/physics.h"
#include "audio/audio.h"
#include "input/input.h"
#include "sensible_nums.h"

// FIXME: move to a more appropriate location?
typedef struct
{
    Graphics *graphics;
    Physics *physics;
    Audio *audio;
    Input *input;
} Resources;

typedef void (*SceneInit)(void *scene_data, Resources *update_state);
// dt is in seconds
typedef void (*SceneUpdate)(void *scene_data, Resources *update_state, f32 dt);
// run once every fixed update (64hz interval)
typedef void (*SceneFixedUpdate)(void *scene_data, Resources *update_state);
typedef void (*SceneFree)(void *scene_data, Resources *update_state);

typedef struct Scene
{
    SceneUpdate update;
    SceneFixedUpdate fixed_update;
    SceneFree free;

    void *scene_data;
} Scene;
