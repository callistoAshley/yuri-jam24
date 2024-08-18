#pragma once

#include "box2d/box2d.h"
#include "sensible_nums.h"

typedef struct
{
    b2WorldId world;
} Physics;

// we use 66hz as the fixed time step (see bevy's reasoning)
#define STEPS_PER_SEC 66

void physics_init(Physics *physics);
void physics_update(Physics *physics);