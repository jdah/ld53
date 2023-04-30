#pragma once

// PRECIOUS CARGO (AND SCARY ALIENS WHO WANT TO TAKE IT!)

#include <cjam/types.h>

// IMPORTANT: CHANGE BEFORE SUBMIT
// TODO
// XXX
// DO THIS
#define WINDOW_SIZE ((ivec2s) {{ 1280, 720 }})
#define TARGET_SIZE ((ivec2s) {{ 192, 108 }})

#define PAUSEKEYS "escape|backspace|."

// TODO TURN OFF
#define HACKS

#define SOUNDS_PER_TICK 2

#define TILE_SIZE_PX 8

#define TICKS_PER_SECOND 30
#define MS_PER_TICK (1000 / TICKS_PER_SECOND)
#define NS_PER_TICK (1000000000 / TICKS_PER_SECOND)

#define LEVEL_WIDTH 19
#define LEVEL_HEIGHT (108 / 8)
#define LEVEL_SIZE ((ivec2s) {{ LEVEL_WIDTH, LEVEL_HEIGHT }})

#define MAX_ENTITIES 16384

#define MAX_TRUCK_HEALTH 100
#define MAX_TRUCK_ARMOR 3
#define MAX_TRUCK_SPEED 3

#define Z_MIN -1.0f
#define Z_MAX 1.0f

#define Z_LEVEL_BASE 0.9f
#define Z_LEVEL_OVERLAY 0.89f
#define Z_LEVEL_ENTITY 0.8f
#define Z_LEVEL_ENTITY_OVERLAY 0.7f
#define Z_LEVEL_PARTICLE 0.6f
#define Z_UI_LEVEL_OVERLAY 0.2f
#define Z_UI 0.5f

#define COLOR_WHITE ((vec4s) {{ 1.0f, 1.0f, 1.0f, 1.0f }})
#define COLOR_BLACK ((vec4s) {{ 0.0f, 0.0f, 0.0f, 0.0f }})

#define START_MONEY 150

typedef enum {
    STAGE_MAIN_MENU,
    STAGE_TITLE,
    STAGE_BUILD,
    STAGE_PLAY,
    STAGE_DONE,
} stage;

typedef enum {
    TILE_NONE = 0,
    TILE_BASE,
    TILE_ROAD,
    TILE_WAREHOUSE_START,
    TILE_WAREHOUSE_FINISH,
    TILE_MOUNTAIN,
    TILE_LAKE,
    TILE_STONE,
    TILE_SLUDGE,
    TILE_MARSH,
    TILE_COUNT
} tile_type;

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
    ENTITY_CANNON_L0,
    ENTITY_CANNON_L1,
    ENTITY_MINE_L0,
    ENTITY_MINE_L1,
    ENTITY_MINE_L2,
    ENTITY_RADAR_L0,
    ENTITY_RADAR_L1,
    ENTITY_BULLET_L0,
    ENTITY_BULLET_L1,
    ENTITY_BULLET_L2,
    ENTITY_SHELL_L0,
    ENTITY_SHELL_L1,
    ENTITY_BOOMBOX,
    ENTITY_START_POINT,
    ENTITY_FLAG,
    ENTITY_DECOY_TRUCK,
    ENTITY_TRUCK,
    ENTITY_ALIEN_L0,
    ENTITY_ALIEN_L1,
    ENTITY_ALIEN_L2,
    ENTITY_ALIEN_FAST,
    ENTITY_ALIEN_TANK,
    ENTITY_ALIEN_GHOST,
    ENTITY_SHIP_L0,
    ENTITY_SHIP_L1,
    ENTITY_SHIP_L2,
    ENTITY_TRANSPORT_L0,
    ENTITY_TRANSPORT_L1,
    ENTITY_TRANSPORT_L2,
    ENTITY_FLAGSHIP,
    ENTITY_REPAIR,
    ENTITY_ARMOR_UPGRADE,
    ENTITY_SPEED_UPGRADE,
    ENTITY_TYPE_COUNT
} entity_type;

// entity info flags
enum {
    EIF_NONE = 0,
    EIF_DOES_NOT_BLOCK = 1 << 0,
    EIF_ENEMY = 1 << 1,
    EIF_PLACEABLE = 1 << 2,
    EIF_ALIEN = 1 << 3,
    EIF_SHIP = 1 << 4,
    EIF_NOT_AN_ENTITY = 1 << 5,
    EIF_CAN_SPAWN = 1 << 6,
};

// level tile flags
enum {
    LTF_NONE = 0,
    LTF_ALIEN_SPAWN = 1 << 0
};

typedef enum {
    PARTICLE_NONE = 0,
    PARTICLE_TEXT,
    PARTICLE_PIXEL,
    PARTICLE_FANCY,
    PARTICLE_MUSIC,
} particle_type;

typedef enum {
    CURSOR_MODE_DEFAULT = 0,
    CURSOR_MODE_DELETE = 1
} cursor_mode;
