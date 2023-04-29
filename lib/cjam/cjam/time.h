#pragma once

#include "types.h"
#include "config.h"

#include <time.h>

#ifdef CJAM_SDL
// nothing to do
#elifdef CJAM_OSX
#   define CJAM_HAS_CLOCK_GETTIME
#elif CJAM_POSIX
#   include <sys/time.h>
#else
#   error TODO
#endif // ifdef CJAM_SDL

ALWAYS_INLINE u64 time_ns() {
#ifdef CJAM_HAS_CLOCK_GETTIME
    struct timespec ts;
    // TODO: is CLOCK_REALTIME cross platform enough?
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * ((u64) 1000000000UL)) + (u64) (ts.tv_nsec);
#elifdef CJAM_SDL
    return SDL_GetTicks64() * 1000;
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return ((u64) (((i64)(now.tv_sec) * 1000) + (now.tv_usec / 1000))) * 1000;
#endif // ifdef CJAM_HAS_CLOCK_GETTIME
}
