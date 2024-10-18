#include "time.h"
#include "utility/time.h"

Time time_new(void) { return (Time){{0}, {0}}; }

void time_advance_by(Time *time, Duration delta)
{
    time->delta = delta;
    time->elapsed = duration_add(time->elapsed, delta);
}
void time_advance_to(Time *time, Duration elapsed)
{
    time_advance_by(time, duration_sub(elapsed, time->elapsed));
}

f32 time_delta_seconds(Time time) { return duration_as_secs_f32(time.delta); }
f32 time_elapsed_seconds(Time time)
{
    return duration_as_secs_f32(time.elapsed);
}
