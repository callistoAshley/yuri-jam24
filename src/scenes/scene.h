#pragma once

#include "graphics/graphics.h"
#include "physics/physics.h"
#include "audio/audio.h"
#include "input/input.h"
#include "sensible_nums.h"
#include "utility/macros.h"

// FIXME: move to a more appropriate location?
typedef struct
{
    Graphics *graphics;
    Physics *physics;
    Audio *audio;
    Input *input;
    Camera *raw_camera;
} Resources;

typedef void (*SceneInit)(void **scene_data, Resources *resources);
// there is no delta passed in directly here- that is on the Input struct!
// use this for handling input, updating game state, etc.
typedef void (*SceneUpdate)(void *scene_data, Resources *resources);
// run once every fixed update (64hz interval)
// this is run after physics updates- use this for things you want to always run
// at a fixed rate, and in-sync with physics
typedef void (*SceneFixedUpdate)(void *scene_data, Resources *resources);
typedef void (*SceneFree)(void *scene_data, Resources *resources);

// other files are expected to provide a constant of this type
typedef struct
{
    SceneInit init;
    SceneUpdate update;
    // this is optional
    NULLABLE SceneFixedUpdate fixed_update;
    SceneFree free;
} SceneInterface;
