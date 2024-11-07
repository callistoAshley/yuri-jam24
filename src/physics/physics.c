#include "physics.h"
#include "box2d/box2d.h"

void physics_init(Physics *physics)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, -9.81f};
    physics->world = b2CreateWorld(&worldDef);
    physics->debug_draw = false;
}

void physics_update(Physics *physics, Time time)
{
    f32 timestep = duration_as_secs(time.delta);
    i32 substeps = 5;

    b2World_Step(physics->world, timestep, substeps);

    b2SensorEvents events = b2World_GetSensorEvents(physics->world);

    for (i32 i = 0; i < events.beginCount; i++)
    {
        b2SensorBeginTouchEvent *event = &events.beginEvents[i];
        SensorUserdata *userdata = b2Shape_GetUserData(event->sensorShapeId);
        if (userdata)
        {
            if (userdata->begin)
            {
                userdata->begin(event, userdata->userdata);
            }
        }
    }

    for (i32 i = 0; i < events.endCount; i++)
    {
        b2SensorEndTouchEvent *event = &events.endEvents[i];
        SensorUserdata *userdata = b2Shape_GetUserData(event->sensorShapeId);
        if (userdata)
        {
            if (userdata->end)
            {
                userdata->end(event, userdata->userdata);
            }
        }
    }
}

void physics_free(Physics *physics) { b2DestroyWorld(physics->world); }
