#pragma once

#include "events/event.h"
#include "fonts/fonts.h"
#include "graphics/graphics.h"
#include "physics/physics.h"
#include "audio/audio.h"
#include "input/input.h"
#include "sensible_nums.h"
#include "settings.h"
#include "time/fixed.h"
#include "time/real.h"
#include "time/time.h"
#include "time/virt.h"
#include "utility/macros.h"

typedef enum
{
    Scene_Map,
    Scene_Title,
} SceneType;

typedef struct
{
    SceneType type;
} Scene;

typedef struct SceneInterface SceneInterface;

// FIXME: move to a more appropriate location?
typedef struct Resources
{

    Graphics *graphics;
    Physics *physics;
    Audio *audio;
    Input *input;
    Camera *raw_camera;
    Fonts *fonts;
    Settings *settings;

    Scene **current_scene;
    SceneInterface *current_scene_interface;

    struct
    {
        // The real (actual) time. Use this if you *need* your code to run at
        // the exact same speed.
        TimeReal *real;
        // A virtual clock. Can be sped up, slowed down, or even paused.
        TimeVirt *virt;
        // A clock that updates at a *fixed rate*.
        TimeFixed *fixed;

        // The current generic time. When performing regualr updates, this will
        // be virtual time. When performing fixed updates (i.e. physics) this
        // will be the fixed time.
        // Most logic should use this instead of real time.
        Time current;
    } time;

    Event *events;
    u32 event_count;

    SDL_Window *window;
} Resources;

typedef void (*SceneInit)(Scene **scene_data, Resources *resources,
                          void *extra_args);

// See
// https://bevy-cheatbook.github.io/fundamentals/fixed-timestep.html#should-i-put-my-systems-in-update-or-fixedupdate
// for more information on these functions

// the generic update function. use this to perform logic that you want to occur
// *every frame*. it is generally a bad idea to use this to update physics (like
// applying forces on objects, for example)
// you should be using this for updating UI, handling input, or animations.
typedef void (*SceneUpdate)(Scene *scene_data, Resources *resources);
// Run every time there is a fixed update.
// Fixed updates are performed at a fixed rate, **which is decoupled from the
// rate at which the game renders at!**
// Logic like AI, physics, and anything else that should be performed
// *consistently* should be done here.
typedef void (*SceneFixedUpdate)(Scene *scene_data, Resources *resources);

// Should free your scene data. It is EXTREMELY important to remove any
// non-persistent things from graphics layers. Failure to do so will likely
// result in the game *crashing*!
typedef void (*SceneFree)(Scene *scene_data, Resources *resources);

// other files are expected to provide a constant of this type
struct SceneInterface
{
    SceneInit init;
    SceneUpdate update;
    // this is optional
    NULLABLE SceneFixedUpdate fixed_update;
    SceneFree free;
};

void scene_change(SceneInterface new_scene, Resources *resources,
                  void *extra_args);
