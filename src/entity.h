#pragma once

#include <cjam/math.h>
#include <cjam/dlist.h>
#include <cjam/dynlist.h>
#include <cjam/aabb.h>

#include "defs.h"
#include "direction.h"

#define E_INFO(_p) (&ENTITY_INFO[(_p)->type])

typedef struct entity_s {
    entity_id id;
    entity_type type;

    vec2s pos;
    ivec2s px;
    ivec2s tile;

    vec2s last_move;
    f32 health, last_health;
    int ticks_alive;

    union {
        struct {
            direction dir;
        } truck;
        struct {
            f32 angle;
            entity_id target;
        } turret;
        struct {
            vec2s velocity;
        } bullet;
        struct {
            direction dir;
            entity_id target;
        } alien;
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

    aabb aabb;
    int palette;
    int max_health;

    struct {
        f32 speed;
        f32 strength;
        int bounty;
    } enemy;

    struct {
        entity_type spawn_type;
        f32 spawns_per_second;
    } ship;

    struct {
        f32 speed;
    } bullet;

    struct {
        entity_type bullet;
        f32 bps;
    } turret;

    struct {
        int radius;
    } radar;

    entity base;
} entity_info;

void entity_set_pos(entity*, vec2s);
aabb entity_aabb(const entity*);
ivec2s entity_center(const entity *e);
void entity_tick(entity*);
void entity_update(entity*, f32 dt);
void entity_draw(entity*);

extern entity_info ENTITY_INFO[ENTITY_TYPE_COUNT];
