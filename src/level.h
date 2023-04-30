#pragma once

#include <cjam/math.h>
#include <cjam/dlist.h>
#include <cjam/dynlist.h>

#include "cjam/aabb.h"
#include "defs.h"

typedef struct entity_s entity;

// TODO
typedef struct {
    const char *title;
    const char *map[LEVEL_HEIGHT];
    struct {
        entity_type type;
        int count;
    } ships[32];
    int bonus;
} level_data;

typedef struct level_s {
    const level_data *data;

    tile_type tiles[LEVEL_WIDTH][LEVEL_HEIGHT];
    int flags[LEVEL_WIDTH][LEVEL_HEIGHT]; // LTF_*
    int music_level[LEVEL_WIDTH][LEVEL_HEIGHT];

    int last_free_entity;
    entity *entities;
    DLIST(entity) tile_entities[LEVEL_WIDTH][LEVEL_HEIGHT];
    DLIST(entity) all_entities;

    ivec2s start, finish;
} level;

void level_init(level*, const level_data *data);
void level_destroy(level*);
bool level_find_near_tile(level*, ivec2s, tile_type, ivec2s*);
void level_go(level*);
void level_tick(level*);
void level_update(level*, f32 dt);
void level_draw(const level*);
void level_update_music(level*);

entity *level_new_entity(level*, entity_type);
void level_delete_entity(level*, entity*);
entity *level_get_entity(level*, entity_id);
entity *level_find_entity(level*, entity_type);
int level_get_tile_entities(level *l, ivec2s tile, entity **es, int n);
int level_get_box_entities(level *l, const aabb *box, entity **es, int n);

typedef int (*f_entity_priority)(entity*, void*);
entity *level_find_nearest_entity(level *l, ivec2s pos, f_entity_priority f_pri, void*);
bool level_tile_has_entities(level*, ivec2s);

int level_path_default_weight(const level *l, ivec2s p, void*);

bool level_path(
    level *level,
    DYNLIST(ivec2s) *dst,
    ivec2s start,
    ivec2s goal,
    int (*weight)(const struct level_s*, ivec2s, void*),
    void *userptr);

ALWAYS_INLINE bool level_tile_in_bounds(ivec2s pos) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < LEVEL_WIDTH && pos.y < LEVEL_HEIGHT;
}

ALWAYS_INLINE bool level_px_in_bounds(ivec2s px) {
    return px.x >= 0 && px.y >= 0 && px.x < LEVEL_WIDTH * TILE_SIZE_PX && px.y < LEVEL_HEIGHT * TILE_SIZE_PX;
}

ALWAYS_INLINE ivec2s level_px_to_tile(ivec2s px) {
    px.x = clamp(px.x, 0, LEVEL_WIDTH * TILE_SIZE_PX);
    px.y = clamp(px.y, 0, LEVEL_HEIGHT * TILE_SIZE_PX);
    return (ivec2s) {{ px.x / TILE_SIZE_PX, px.y / TILE_SIZE_PX }};
}

ALWAYS_INLINE ivec2s level_tile_to_px(ivec2s pos) {
    return (ivec2s) {{ pos.x * TILE_SIZE_PX, pos.y * TILE_SIZE_PX }};
}

ALWAYS_INLINE ivec2s level_px_round_to_tile(ivec2s pos) {
    return level_tile_to_px(level_px_to_tile(pos));
}

ALWAYS_INLINE ivec2s level_tile_center_px(ivec2s pos) {
    const ivec2s px = level_tile_to_px(pos);
    return (ivec2s) {{ px.x + (TILE_SIZE_PX / 2), px.y + (TILE_SIZE_PX / 2) }};
}

ALWAYS_INLINE ivec2s level_clamp_tile(ivec2s pos) {
    return (ivec2s) {{ clamp(pos.x, 0, LEVEL_WIDTH - 1), clamp(pos.y, 0, LEVEL_HEIGHT - 1) }};
}
