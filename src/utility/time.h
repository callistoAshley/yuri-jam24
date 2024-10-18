#pragma once

#include "sensible_nums.h"

// Somewhat inspired by Rust's std::time::Duration, this represents a span of
// time.
// Durations are represented as nanoseconds.
typedef struct
{
    // this field is not intended to be used directly!
    u64 nanos;
} Duration;

Duration duration_new(u64 nanos);
Duration duration_from_secs(u64 secs);
Duration duration_from_millis(u64 millis);
Duration duration_from_micros(u64 micros);

Duration duration_from_secs_f64(f64 secs);

f32 duration_as_secs_f32(Duration duration);
f64 duration_as_secs_f64(Duration duration);

Duration duration_sub(Duration duration, Duration other);
Duration duration_add(Duration duration, Duration other);

// Instants are inspired by Rust's std::time::Instant, representing a fixed
// point in time. Barring platform bugs the should be nondecreasing.
typedef struct
{
    // this field is not intended to be used directly!
    Duration dur;
} Instant;

Instant instant_now(void);

Duration instant_elapsed(Instant instant);
Duration instant_duration_since(Instant instant, Instant earlier);

Duration instant_sub(Instant instant, Instant earlier);
Instant instant_sub_dur(Instant instant, Duration time);
Instant instant_add(Instant instant, Duration time);
