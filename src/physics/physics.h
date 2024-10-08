#pragma once

#include "box2d/box2d.h"
#include "sensible_nums.h"

typedef struct
{
    b2WorldId world;
    bool debug_draw;
} Physics;

// FIXME jank
typedef void (*sensor_begin_touch_fn)(b2SensorBeginTouchEvent *event,
                                      void *userdata);
typedef void (*sensor_end_touch_fn)(b2SensorEndTouchEvent *event,
                                    void *userdata);

typedef struct
{
    sensor_begin_touch_fn begin;
    sensor_end_touch_fn end;
    void *userdata;
} SensorUserdata;

void physics_init(Physics *physics);
void physics_update(Physics *physics);
