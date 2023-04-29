#pragma once

#include <cjam/math.h>

#include "defs.h"

typedef enum {
    TILE_NONE = 0,
    TILE_BASE,
    TILE_ROAD,
    TILE_COUNT
} tile_type;

typedef enum {
    OBJECT_NONE = 0,
    OBJECT_TURRET_L0,
    OBJECT_COUNT
} object_type;

typedef struct level_s {
    tile_type tiles[LEVEL_WIDTH][LEVEL_HEIGHT];
    object_type objects[LEVEL_WIDTH][LEVEL_HEIGHT];
} level;

void level_init(level*);
void level_draw(const level*);

ALWAYS_INLINE bool level_tile_in_bounds(ivec2s pos) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < LEVEL_WIDTH && pos.y < LEVEL_HEIGHT;
}

ALWAYS_INLINE bool level_px_in_bounds(ivec2s px) {
    return px.x >= 0 && px.y >= 0 && px.x < LEVEL_WIDTH * TILE_SIZE_PX && px.y < LEVEL_HEIGHT * TILE_SIZE_PX;
}

ALWAYS_INLINE ivec2s level_px_to_tile(ivec2s px) {
    px.x = clamp(px.x, 0, LEVEL_WIDTH * TILE_SIZE_PX);
    px.y = clamp(px.y, 0, LEVEL_WIDTH * TILE_SIZE_PX);
    return (ivec2s) {{ px.x / TILE_SIZE_PX, px.y / TILE_SIZE_PX }};
}

ALWAYS_INLINE ivec2s level_tile_to_px(ivec2s pos) {
    return (ivec2s) {{ pos.x * TILE_SIZE_PX, pos.y * TILE_SIZE_PX }};
}

ALWAYS_INLINE ivec2s level_px_round_to_tile(ivec2s pos) {
    return level_tile_to_px(level_px_to_tile(pos));
}
