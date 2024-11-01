extern "C"
{
#include "time.h"
}
#include <chrono>
#include <cstring>

typedef std::chrono::nanoseconds duration_inner;
typedef std::chrono::time_point<std::chrono::steady_clock> instant_inner;

extern "C"
{
    Duration duration_new(u64 nanos)
    {
        duration_inner inner(nanos);
        return {inner.count()};
    }

    Duration duration_from_secs(u64 secs)
    {
        std::chrono::seconds seconds(secs);
        duration_inner inner(seconds);
        return {inner.count()};
    }

    Duration duration_from_millis(u64 millis)
    {
        std::chrono::milliseconds milliseconds(millis);
        duration_inner inner(milliseconds);
        return {inner.count()};
    }

    Duration duration_from_micros(u64 micros)
    {
        std::chrono::microseconds microseconds(micros);
        duration_inner inner(microseconds);
        return {inner.count()};
    }

    Duration duration_from_secs_f64(f64 secs)
    {
        std::chrono::duration<f64> seconds(secs);
        duration_inner inner =
            std::chrono::duration_cast<duration_inner>(seconds);
        return {inner.count()};
    }

    f32 duration_as_secs(Duration duration)
    {
        duration_inner inner(duration.inner);
        std::chrono::duration<f32> seconds(inner);
        return seconds.count();
    }

    f64 duration_as_secs_f64(Duration duration)
    {
        duration_inner inner(duration.inner);
        std::chrono::duration<f64> seconds(inner);
        return seconds.count();
    }

    Duration duration_sub(Duration duration, Duration other)
    {
        duration_inner inner(duration.inner);
        duration_inner inner2(other.inner);
        duration_inner result = inner - inner2;
        return {result.count()};
    }

    Duration duration_add(Duration duration, Duration other)
    {
        duration_inner inner(duration.inner);
        duration_inner inner2(other.inner);
        duration_inner result = inner + inner2;
        return {result.count()};
    }

    Duration duration_mul_f32(Duration duration, f32 by)
    {
        duration_inner inner(duration.inner);
        duration_inner result =
            std::chrono::duration_cast<duration_inner>(inner * by);
        return {result.count()};
    }

    Duration duration_mul_f64(Duration duration, f64 by)
    {
        duration_inner inner(duration.inner);
        duration_inner result =
            std::chrono::duration_cast<duration_inner>(inner * by);
        return {result.count()};
    }

    bool duration_is_lt(Duration duration, Duration other)
    {
        duration_inner inner(duration.inner);
        duration_inner inner2(other.inner);
        return inner < inner2;
    }

    bool duration_is_gt(Duration duration, Duration other)
    {
        duration_inner inner(duration.inner);
        duration_inner inner2(other.inner);
        return inner > inner2;
    }

    Instant instant_now()
    {
        instant_inner now = std::chrono::steady_clock::now();
        return {now.time_since_epoch().count()};
    }

    Duration instant_elapsed(Instant instant)
    {
        instant_inner::duration instant_dur(instant.inner);
        duration_inner duration(instant_dur);
        return {duration.count()};
    }

    Duration instant_duration_since(Instant instant, Instant earlier)
    {
        instant_inner::duration instant_dur(instant.inner);
        instant_inner inner(instant_dur);

        instant_inner::duration earlier_dur(earlier.inner);
        instant_inner earlier_inner(earlier_dur);

        duration_inner duration = inner - earlier_inner;
        return {duration.count()};
    }

    Duration instant_sub(Instant instant, Instant earlier)
    {
        return instant_duration_since(instant, earlier);
    }

    Instant instant_sub_dur(Instant instant, Duration time)
    {
        instant_inner::duration instant_dur(instant.inner);
        instant_inner inner(instant_dur);

        duration_inner duration(time.inner);

        instant_inner result = inner - duration;
        return {result.time_since_epoch().count()};
    }

    Instant instant_add(Instant instant, Duration time)
    {
        instant_inner::duration instant_dur(instant.inner);
        instant_inner inner(instant_dur);

        duration_inner duration(time.inner);

        instant_inner result = inner + duration;
        return {result.time_since_epoch().count()};
    }
}