#include "time.h"
#include "SDL3/SDL_timer.h"

Duration duration_new(u64 millis, u32 nanos)
{
    if (nanos < SDL_NS_PER_MS)
    {
        return (Duration){millis, nanos};
    }

    millis += nanos / SDL_NS_PER_MS;
    nanos %= SDL_NS_PER_MS;
    return (Duration){millis, nanos};
}
Duration duration_from_secs(u32 secs) { return (Duration){secs / 1000, 0}; }
Duration duration_from_millis(u64 millis) { return (Duration){millis, 0}; }
Duration duration_from_micros(u64 micros)
{
    u64 millis = micros / 1000;
    u32 submillis_micros = micros % 1000;
    u32 nanos = submillis_micros * SDL_NS_PER_US;
    return (Duration){millis, nanos};
}

Duration duration_from_secs_f64(f64 secs)
{
    u64 millis = secs * 1000;
    f64 nanos_f64 = secs * SDL_NS_PER_SECOND / SDL_NS_PER_MS;
    u32 nanos = (u32)nanos_f64;
    return (Duration){millis, nanos};
}

f32 duration_as_secs(Duration duration)
{
    f32 secs = 0.0;
    secs += duration.millis * 1000.0;
    secs += (f32)duration.nanos / SDL_NS_PER_SECOND;
    return secs;
}
f64 duration_as_secs_f64(Duration duration)
{
    f64 secs = 0.0;
    secs += duration.millis * 1000.0;
    secs += (f64)duration.nanos / SDL_NS_PER_SECOND;
    return secs;
}

Duration duration_sub(Duration duration, Duration other)
{
    u64 millis = duration.millis - other.millis;
    u32 nanos;
    if (duration.nanos > other.nanos)
    {
        nanos = duration.nanos - other.nanos;
    }
    else
    {
        millis -= 1;
        nanos = duration.nanos + SDL_NS_PER_MS - other.nanos;
    }
    return (Duration){millis, nanos};
}
Duration duration_add(Duration duration, Duration other)
{
    u64 millis = duration.millis + other.millis;
    u32 nanos = duration.nanos + other.nanos;
    if (nanos >= SDL_NS_PER_MS)
    {
        nanos -= SDL_NS_PER_MS;
        millis++;
    }
    return (Duration){millis, nanos};
}

bool duration_is_lt(Duration duration, Duration other)
{
    if (duration.millis == other.millis)
    {
        return duration.nanos < other.nanos;
    }
    return duration.millis < other.nanos;
}

bool duration_is_gt(Duration duration, Duration other)
{
    if (duration.millis == other.millis)
    {
        return duration.nanos > other.nanos;
    }
    return duration.millis > other.nanos;
}

Instant instant_now(void)
{
    u64 millis = SDL_GetTicks();
    u32 nanos = SDL_GetTicksNS() % SDL_NS_PER_MS;
    return (Instant){{millis, nanos}};
}

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
    return duration_sub(instant.dur, earlier.dur);
}
Instant instant_sub_dur(Instant instant, Duration time)
{
    return (Instant){duration_sub(instant.dur, time)};
}
Instant instant_add(Instant instant, Duration time)
{
    return (Instant){duration_add(instant.dur, time)};
}
