#pragma once

#include "time/time.h"
#include "utility/time.h"
#include <stdbool.h>

typedef struct
{
    Time time;

    Duration timestep, overstep;
} TimeFixed;

#define FIXED_DEFAULT_TIMESTEP duration_from_micros(15625)

TimeFixed time_fixed_new(void);
TimeFixed time_fixed_from_dur(Duration timestep);
TimeFixed time_fixed_from_seconds(f64 secs);
TimeFixed time_fixed_from_hz(f64 hz);

void time_fixed_set_step_secs(TimeFixed *time, f64 secs);
void time_fixed_set_step_hz(TimeFixed *time, f64 hz);

void time_fixed_discard_overstep(TimeFixed *time, Duration discard);
void time_fixed_accumulate(TimeFixed *time, Duration delta);
bool time_fixed_expend(TimeFixed *time);
