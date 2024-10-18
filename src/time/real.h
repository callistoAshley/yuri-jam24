#pragma once

#include "time/time.h"
#include "utility/time.h"
#include <stdbool.h>

typedef struct
{
    Time time;

    Instant startup;
    Instant first_update, last_update;
    bool has_updated;
} TimeReal;

TimeReal time_real_new(void);
TimeReal time_real_new_with(Instant startup);

void time_real_update(TimeReal *time);
void time_real_update_with(TimeReal *time, Instant inst);
void time_real_update_with_dur(TimeReal *time, Duration dur);
