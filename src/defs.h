#pragma once

#include <cjam/types.h>

// IMPORTANT: CHANGE BEFORE SUBMIT
// TODO
// XXX
// DO THIS
#define WINDOW_SIZE ((ivec2s) {{ 1280, 720 }})
#define TARGET_SIZE ((ivec2s) {{ 192, 108 }})

#define TILE_SIZE_PX 8

#define TICKS_PER_SECOND 30
#define MS_PER_TICK (1000 / TICKS_PER_SECOND)
#define NS_PER_TICK (1000000000 / TICKS_PER_SECOND)

#define LEVEL_WIDTH 20
#define LEVEL_HEIGHT (108 / 8)
#define LEVEL_SIZE ((ivec2s) {{ LEVEL_WIDTH, LEVEL_HEIGHT }})

#define MAX_ENTITIES 16384

#define Z_MIN -1.0f
#define Z_MAX 1.0f

#define Z_LEVEL_BASE 0.9f
#define Z_LEVEL_OVERLAY 0.89f
#define Z_LEVEL_ENTITY 0.8f
#define Z_LEVEL_ENTITY_OVERLAY 0.7f
#define Z_UI 0.0f

#define COLOR_WHITE ((vec4s) {{ 1.0f, 1.0f, 1.0f, 1.0f }})
#define COLOR_BLACK ((vec4s) {{ 0.0f, 0.0f, 0.0f, 0.0f }})

typedef enum {
    GAME_STATE_MAIN_MENU,
    GAME_STATE_BUILD,
    GAME_STATE_PLAY,
    GAME_STATE_DONE,
} game_state;

typedef enum {
    TILE_NONE = 0,
    TILE_BASE,
    TILE_ROAD,
    TILE_COUNT
} tile_type;

typedef enum {
    OBJECT_NONE = 0,
    OBJECT_TURRET_L0,
    OBJECT_TURRET_L1,
    OBJECT_COUNT
} object_type;

typedef struct {
    bool present: 1;
    u16 gen: 15;
    u16 index;
} entity_id;

#define ENTITY_NONE ((entity_id) { 0 })

typedef enum {
    ENTITY_TYPE_NONE = 0,
    ENTITY_TURRET_L0,
    ENTITY_TURRET_L1,
    ENTITY_TURRET_L2,
    ENTITY_BULLET_L0,
    ENTITY_START_POINT,
    ENTITY_FLAG,
    ENTITY_TRUCK,
    ENTITY_TYPE_COUNT
} entity_type;
