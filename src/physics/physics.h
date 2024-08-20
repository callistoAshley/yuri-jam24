#pragma once

#include "box2d/box2d.h"
#include "sensible_nums.h"

typedef struct
{
    b2WorldId world;
} Physics;

// we use 64hz as the fixed time step (see bevy's reasoning)
#define STEPS_PER_SEC 64

// pixels per meter
#define PX_PER_M 16
// convert between pixels and meters
#define PX_TO_M(x) ((x) / PX_PER_M)
// convert between meters and pixels
#define M_TO_PX(x) ((x) * PX_PER_M)

void physics_init(Physics *physics);
void physics_update(Physics *physics);
