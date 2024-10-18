#include "time.h"
#include "SDL3/SDL_timer.h"

Duration duration_new(u64 nanos) { return (Duration){nanos}; }
Duration duration_from_secs(u64 secs)
{
    return (Duration){SDL_SECONDS_TO_NS(secs)};
}
Duration duration_from_millis(u64 millis)
{
    return (Duration){SDL_MS_TO_NS(millis)};
}
Duration duration_from_micros(u64 micros)
{
    return (Duration){SDL_US_TO_NS(micros)};
}

Duration duration_from_secs_f64(f64 secs)
{
    return (Duration){SDL_SECONDS_TO_NS(secs)};
}

f32 duration_as_secs(Duration duration)
{
    return SDL_NS_TO_SECONDS((f32)duration.nanos);
}
f64 duration_as_secs_f64(Duration duration)
{
    return SDL_NS_TO_SECONDS((f64)duration.nanos);
}

Duration duration_sub(Duration duration, Duration other)
{
    return (Duration){duration.nanos - other.nanos};
}
Duration duration_add(Duration duration, Duration other)
{
    return (Duration){duration.nanos + other.nanos};
}

Instant instant_now(void) { return (Instant){{SDL_GetTicksNS()}}; }

Duration instant_elapsed(Instant instant)
{
    Instant now = instant_now();
    return instant_duration_since(now, instant);
}
Duration instant_duration_since(Instant instant, Instant earlier)
{
    return instant_sub(instant, earlier);
}

Duration instant_sub(Instant instant, Instant earlier)
{
    return (Duration){instant.dur.nanos - earlier.dur.nanos};
}
Instant instant_sub_dur(Instant instant, Duration time)
{
    return (Instant){{instant.dur.nanos - time.nanos}};
}
Instant instant_add(Instant instant, Duration time)
{
    return (Instant){{instant.dur.nanos + time.nanos}};
}
