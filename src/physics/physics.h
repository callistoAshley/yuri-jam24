#pragma once

#include "box2d/box2d.h"
#include "sensible_nums.h"

typedef struct
{
    b2WorldId world;
    bool debug_draw;
} Physics;

void physics_init(Physics *physics);
void physics_update(Physics *physics);
