#pragma once

#include "events/event.h"
#include "fonts/fonts.h"
#include "graphics/graphics.h"
#include "items/item.h"
#include "physics/physics.h"
#include "audio/audio.h"
#include "input/input.h"
#include "sensible_nums.h"
#include "settings.h"
#include "time/fixed.h"
#include "time/real.h"
#include "time/time.h"
#include "time/virt.h"
#include "scenes/scene.h"

#define INVENTORY_SIZE 16

typedef struct Resources
{
    Graphics graphics;
    Physics physics;
    Audio audio;
    Input input;
    Camera raw_camera;
    Fonts fonts;
    Settings settings;

    // all scenes are required to start with SceneType as their first field.
    // use that to check what the current scene is rather than checking the
    // interface
    Scene *scene;
    SceneInterface scene_interface;

    struct
    {
        // The real (actual) time. Use this if you *need* your code to run at
        // the exact same speed.
        TimeReal real;
        // A virtual clock. Can be sped up, slowed down, or even paused.
        TimeVirt virt;
        // A clock that updates at a *fixed rate*.
        TimeFixed fixed;

        // The current generic time. When performing regualr updates, this will
        // be virtual time. When performing fixed updates (i.e. physics) this
        // will be the fixed time.
        // Most logic should use this instead of real time.
        Time current;
    } time;

    Event *events;
    u32 event_count;

    NULLABLE const Item *inventory[INVENTORY_SIZE];

    SDL_Window *window;
} Resources;
