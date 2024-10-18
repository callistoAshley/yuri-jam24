#pragma once

#include "time/time.h"
#include "utility/time.h"
#include <stdbool.h>

typedef struct
{
    Time time;

    Duration max_delta;
    bool paused;

    f64 relative_speed, effective_speed;
} TimeVirt;

#define TIME_VIRT_MAX_DELTA duration_from_millis(250)

TimeVirt time_virt_new(void);
TimeVirt time_virt_from_max_delta(Duration delta);

void time_virt_advance_with(TimeVirt *time, Duration raw_delta);
