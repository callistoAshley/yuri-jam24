#pragma once

#include "fonts/fonts.h"
#include "graphics/graphics.h"
#include "physics/physics.h"
#include "audio/audio.h"
#include "input/input.h"
#include "sensible_nums.h"
#include "settings.h"
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
typedef struct
{
    bool debug_mode;

    Graphics *graphics;
    Physics *physics;
    Audio *audio;
    Input *input;
    Camera *raw_camera;
    Fonts *fonts;
    Settings *settings;

    // only used to modify the current scene
    Scene **current_scene;
    SceneInterface *current_scene_interface;

    SDL_Window *window;
} Resources;

typedef void (*SceneInit)(Scene **scene_data, Resources *resources,
                          void *extra_args);
// there is no delta passed in directly here- that is on the Input struct!
// use this for handling input, updating game state, etc.
typedef void (*SceneUpdate)(Scene *scene_data, Resources *resources);
// run once every fixed update (64hz interval)
// this is run after physics updates- use this for things you want to always run
// at a fixed rate, and in-sync with physics
typedef void (*SceneFixedUpdate)(Scene *scene_data, Resources *resources);
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
