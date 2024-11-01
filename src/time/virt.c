#include "virt.h"
#include "time/time.h"
#include "utility/time.h"

TimeVirt time_virt_new(void)
{
    TimeVirt time = {
        .time = time_new(),
        .max_delta = TIME_VIRT_MAX_DELTA,
        .paused = false,
        .relative_speed = 1.0,
        .effective_speed = 1.0,
    };
    return time;
}
TimeVirt time_virt_from_max_delta(Duration delta)
{
    TimeVirt time = time_virt_new();
    time.max_delta = delta;
    return time;
}

void time_virt_advance_with(TimeVirt *time, Duration raw_delta)
{
    Duration max_delta = time->max_delta;

    Duration clamped_delta = raw_delta;
    if (duration_is_gt(raw_delta, max_delta))
        clamped_delta = max_delta;

    f64 effective_speed = time->relative_speed;
    if (time->paused)
        effective_speed = 0.0;

    Duration delta = clamped_delta;
    if (effective_speed != 1.0)
    {
        delta = duration_mul_f64(clamped_delta, effective_speed);
    }

    time->effective_speed = effective_speed;
    time_advance_by(&time->time, delta);
}
