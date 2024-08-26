#include "physics.h"
#include "utility/common_defines.h"

void physics_init(Physics *physics)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, -9.81f};
    physics->world = b2CreateWorld(&worldDef);
    physics->debug_draw = false;
}

void physics_update(Physics *physics)
{
    f32 timestep = 1.0f / FIXED_STEPS_PER_SEC;
    i32 substeps = 5;

    b2World_Step(physics->world, timestep, substeps);
}
