#pragma once

#include "utility/macros.h"

typedef enum
{
    Scene_Map,
    Scene_Title,
    Scene_FmodLogo,
} SceneType;

typedef struct
{
    SceneType type;
} Scene;

typedef struct Resources Resources;

// SceneInit is expected to set resources->current_scene
typedef void (*SceneInit)(Resources *resources, void *extra_args);
// See
// https://bevy-cheatbook.github.io/fundamentals/fixed-timestep.html#should-i-put-my-systems-in-update-or-fixedupdate
// for more information on these functions
// the generic update function. use this to perform logic that you want to occur
// *every frame*. it is generally a bad idea to use this to update physics (like
// applying forces on objects, for example)
// you should be using this for updating UI, handling input, or animations.
typedef void (*SceneUpdate)(Resources *resources);
// Run every time there is a fixed update.
// Fixed updates are performed at a fixed rate, **which is decoupled from the
// rate at which the game renders at!**
// Logic like AI, physics, and anything else that should be performed
// *consistently* should be done here.
typedef void (*SceneFixedUpdate)(Resources *resources);

// Should free your scene data. It is EXTREMELY important to remove any
// non-persistent things from graphics layers. Failure to do so will likely
// result in the game *crashing*!
typedef void (*SceneFree)(Resources *resources);

// other files are expected to provide a constant of this type
typedef struct SceneInterface
{
    SceneInit init;
    SceneUpdate update;
    // this is optional
    NULLABLE SceneFixedUpdate fixed_update;
    SceneFree free;
} SceneInterface;

void scene_change(SceneInterface new_scene, Resources *resources,
                  void *extra_args);
