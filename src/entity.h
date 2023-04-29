#pragma once

#include <cjam/math.h>
#include <cjam/dlist.h>
#include <cjam/dynlist.h>

#include "defs.h"

typedef struct entity_s {
    entity_id id;
    entity_type type;

    vec2s pos;
    ivec2s px;
    ivec2s tile;

    union {
        struct {
            f32 angle;
        } turret;
        struct {
            vec2s velocity;
        } bullet;
    };

    // path, likely not present
    DYNLIST(ivec2s) path;

    bool delete;

    // see level.h
    DLIST_NODE(struct entity_s) node;

    bool on_tile;
    DLIST_NODE(struct entity_s) tile_node;
} entity;

typedef void (*f_entity_draw)(entity*);
typedef void (*f_entity_tick)(entity*);
typedef void (*f_entity_update)(entity*, f32);
typedef bool (*f_entity_can_place)(ivec2s);

typedef struct entity_info_s {
    const char *name;
    ivec2s base_sprite;
    f_entity_draw draw;
    f_entity_tick tick;
    f_entity_update update;
    f_entity_can_place can_place;
    int flags; // EIF_*
    int unlock_price, buy_price;
} entity_info;

void entity_set_pos(entity*, vec2s);
void entity_tick(entity*);
void entity_update(entity*, f32 dt);
void entity_draw(entity*);

extern entity_info ENTITY_INFO[ENTITY_TYPE_COUNT];