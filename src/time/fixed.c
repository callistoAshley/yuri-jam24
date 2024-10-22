#include "fixed.h"
#include "time/time.h"
#include "utility/time.h"

TimeFixed time_fixed_new(void)
{
    TimeFixed time = {
        .time = time_new(),
        .timestep = FIXED_DEFAULT_TIMESTEP,
        .overstep = {0},
    };
    return time;
}
TimeFixed time_fixed_from_dur(Duration timestep)
{
    TimeFixed time = time_fixed_new();
    time.timestep = timestep;
    return time;
}
TimeFixed time_fixed_from_seconds(f64 secs)
{
    TimeFixed time = time_fixed_new();
    time_fixed_set_step_secs(&time, secs);
    return time;
}
TimeFixed time_fixed_from_hz(f64 hz)
{
    TimeFixed time = time_fixed_new();
    time_fixed_set_step_hz(&time, hz);
    return time;
}

void time_fixed_set_step_secs(TimeFixed *time, f64 secs)
{
    time->timestep = duration_from_secs_f64(secs);
}
void time_fixed_set_step_hz(TimeFixed *time, f64 hz)
{
    time_fixed_set_step_secs(time, 1.0 / hz);
}

f32 time_fixed_overstep_fraction(TimeFixed *time)
{
    return duration_as_secs(time->overstep) / duration_as_secs(time->timestep);
}

void time_fixed_discard_overstep(TimeFixed *time, Duration discard)
{
    // FIXME make this saturate
    time->overstep = duration_sub(time->overstep, discard);
}
void time_fixed_accumulate(TimeFixed *time, Duration delta)
{
    time->overstep = duration_add(time->overstep, delta);
}
bool time_fixed_expend(TimeFixed *time)
{
    if (time->overstep.nanos > time->timestep.nanos)
    {
        time->overstep = duration_sub(time->overstep, time->timestep);
        time_advance_by(&time->time, time->timestep);
        return true;
    }
    else
    {
        return false;
    }
}
