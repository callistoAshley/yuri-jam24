#pragma once

#include "sensible_nums.h"

typedef struct
{
    i64 inner;
} Duration;

Duration duration_new(u64 nanos);
Duration duration_from_secs(u64 secs);
Duration duration_from_millis(u64 millis);
Duration duration_from_micros(u64 micros);

Duration duration_from_secs_f64(f64 secs);

f32 duration_as_secs(Duration duration);
f64 duration_as_secs_f64(Duration duration);

Duration duration_sub(Duration duration, Duration other);
Duration duration_add(Duration duration, Duration other);

Duration duration_mul_f32(Duration duration, f32 by);
Duration duration_mul_f64(Duration duration, f64 by);

bool duration_is_lt(Duration duration, Duration other);
bool duration_is_gt(Duration duration, Duration other);

typedef struct
{
    i64 inner;
} Instant;

Instant instant_now(void);

Duration instant_elapsed(Instant instant);
Duration instant_duration_since(Instant instant, Instant earlier);

Duration instant_sub(Instant instant, Instant earlier);
Instant instant_sub_dur(Instant instant, Duration time);
Instant instant_add(Instant instant, Duration time);
