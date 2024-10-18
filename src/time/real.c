#include "real.h"
#include "time/time.h"
#include "utility/time.h"

TimeReal time_real_new(void) { return time_real_new_with(instant_now()); }
TimeReal time_real_new_with(Instant startup)
{
    TimeReal real = {
        .time = time_new(),
        .startup = startup,
        .has_updated = false,
    };
    return real;
}

void time_real_update(TimeReal *time)
{
    Instant now = instant_now();
    time_real_update_with(time, now);
}
void time_real_update_with(TimeReal *time, Instant inst)
{
    if (!time->has_updated)
    {
        time->first_update = inst;
        time->last_update = inst;
        time->has_updated = true;
        return;
    }

    Duration delta = instant_sub(inst, time->last_update);
    time_advance_by(&time->time, delta);
    time->last_update = inst;
}
void time_real_update_with_dur(TimeReal *time, Duration dur)
{
    Instant last_update = time->startup;
    if (time->has_updated)
        last_update = time->last_update;

    Instant inst = instant_add(last_update, dur);
    time_real_update_with(time, inst);
}
