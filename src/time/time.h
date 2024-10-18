#pragma once

#include "utility/time.h"
typedef struct
{
    Duration delta;
    Duration elapsed;
} Time;

Time time_new(void);

void time_advance_by(Time *time, Duration delta);
void time_advance_to(Time *time, Duration elapsed);

f32 time_delta_seconds(Time time);
f32 time_elapsed_seconds(Time time);
