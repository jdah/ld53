#pragma once

#include <cjam/math.h>
#include <cjam/types.h>

typedef enum {
    // cardinal dirs
    DIRECTION_NORTH           = 0,
    DIRECTION_SOUTH           = 1,
    DIRECTION_EAST            = 2,
    DIRECTION_WEST            = 3,

    DIRECTION_UP              = DIRECTION_NORTH,
    DIRECTION_DOWN            = DIRECTION_SOUTH,
    DIRECTION_LEFT            = DIRECTION_EAST,
    DIRECTION_RIGHT           = DIRECTION_WEST,

    DIRECTION_CARDINAL_COUNT  = 4,
    DIRECTION_FIRST = 0,

    // intercardinal dirs
    DIRECTION_NORTHEAST       = 4,
    DIRECTION_NORTHWEST       = 5,
    DIRECTION_SOUTHEAST       = 6,
    DIRECTION_SOUTHWEST       = 7,

    DIRECTION_UP_LEFT         = 4,
    DIRECTION_UP_RIGHT        = 5,
    DIRECTION_DOWN_LEFT       = 6,
    DIRECTION_DOWN_RIGHT      = 7,

    // number of cardinal + ordinal dirs
    DIRECTION_ALL_COUNT       = 8,
} direction;

typedef enum {
    DIRECTION_FLAG_NORTH      = 1 << DIRECTION_NORTH,
    DIRECTION_FLAG_SOUTH      = 1 << DIRECTION_SOUTH,
    DIRECTION_FLAG_EAST       = 1 << DIRECTION_EAST,
    DIRECTION_FLAG_WEST       = 1 << DIRECTION_WEST,

    DIRECTION_FLAG_UP         = DIRECTION_FLAG_NORTH,
    DIRECTION_FLAG_DOWN       = DIRECTION_FLAG_SOUTH,
    DIRECTION_FLAG_LEFT       = DIRECTION_FLAG_EAST,
    DIRECTION_FLAG_RIGHT      = DIRECTION_FLAG_WEST,

    DIRECTION_FLAG_NORTHEAST  = 1 << DIRECTION_NORTHEAST,
    DIRECTION_FLAG_NORTHWEST  = 1 << DIRECTION_NORTHWEST,
    DIRECTION_FLAG_SOUTHEAST  = 1 << DIRECTION_SOUTHEAST,
    DIRECTION_FLAG_SOUTHWEST  = 1 << DIRECTION_SOUTHWEST,

    DIRECTION_FLAG_UP_LEFT    = DIRECTION_FLAG_NORTHEAST,
    DIRECTION_FLAG_UP_RIGHT   = DIRECTION_FLAG_NORTHWEST,
    DIRECTION_FLAG_DOWN_LEFT  = DIRECTION_FLAG_SOUTHEAST,
    DIRECTION_FLAG_DOWN_RIGHT = DIRECTION_FLAG_SOUTHWEST,
} direction_flags;

ALWAYS_INLINE direction direction_from_vec2s(vec2s v, direction if_zero) {
    if (glms_vec2_norm(v) < 0.00000001f) { return if_zero; }
    return fabs(v.x) > fabs(v.y) ?
        (v.x > 0.0f ? DIRECTION_WEST : DIRECTION_EAST)
        : (v.y > 0.0f ? DIRECTION_UP : DIRECTION_DOWN);
}

ALWAYS_INLINE ivec2s direction_to_ivec2s(direction d) {
    return (
        (ivec2s[DIRECTION_CARDINAL_COUNT]) {
            (ivec2s) {{ 0, -1 }},
            (ivec2s) {{ 0, 1 }},
            (ivec2s) {{ -1, 0 }},
            (ivec2s) {{ 1, 0 }}
        })[d];
}
