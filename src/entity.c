#include "entity.h"
#include "gfx.h"
#include "level.h"
#include "state.h"

#include <cjam/time.h>

void entity_set_pos(entity *e, vec2s pos) {
    const ivec2s new_tile = level_px_to_tile((ivec2s) {{ pos.x, pos.y }});

    const bool is_new_tile =
        !e->on_tile
        || e->tile.x != new_tile.x
        || e->tile.y != new_tile.y;

    if (e->on_tile && is_new_tile) {
        dlist_remove(
            tile_node,
            &state->level->tile_entities[e->tile.x][e->tile.y],
            e);
    }

    e->pos = pos;
    e->px = (ivec2s) {{ roundf(pos.x), roundf(pos.y) }};
    e->tile = new_tile;
    e->on_tile = level_tile_in_bounds(e->tile);

    if (is_new_tile && e->on_tile) {
        ASSERT(e->tile_node.next == NULL && e->tile_node.prev == NULL);
        ASSERT(e->on_tile);
        dlist_append(
            tile_node,
            &state->level->tile_entities[e->tile.x][e->tile.y],
            e);
    }
}

int truck_path_weight(const level *l, ivec2s p, void*) {
    if (!level_tile_in_bounds(p)) {
        return -1;
    } else if (l->tiles[p.x][p.y] != TILE_ROAD) {
        return -1;
    }

    return 1;
}

static void tick_turret(entity *e){
    if (state->time.tick % 2 != 0) { return; }
    entity *bullet = level_new_entity(state->level);
    bullet->type = ENTITY_BULLET_L0;
    const f32 speed = 64.0f;
    bullet->bullet.velocity = (vec2s) {{
        speed * -cos(e->turret.angle), speed * -sin(e->turret.angle),
    }};
    entity_set_pos(bullet, (vec2s) {{ e->pos.x + 4, e->pos.y + 5 }});
}

static void tick_truck(entity *e) {
    dynlist_free(e->path);

    entity *flag = level_find_entity(state->level, ENTITY_FLAG);
    ASSERT(flag);

    const bool success =
        level_path(
            state->level,
            &e->path,
            e->tile,
            flag->tile,
            truck_path_weight,
            NULL);

    if (!success) {
        dynlist_free(e->path);
        e->path = NULL;
    }

    // move to next path point
    if (success) {
        /* if (dynlist_size(e->path) == 1) { */
        /*     state->state = GAME_STATE_DONE; */
        /* } else { */
        /*     const ivec2s */
        /*         next = dynlist_size(e->path) >= 2 ? e->path[1] : e->path[0], */
        /*         next_px = level_tile_to_px(next); */

        /*     const vec2s dir = */
        /*         glms_vec2_normalize((vec2s) {{ */
        /*             next_px.x - e->pos.x, */
        /*             next_px.y - e->pos.y */
        /*         }}); */

        /*     const f32 speed = 2.0f; */
        /*     entity_set_pos( */
        /*         e, */
        /*         (vec2s) {{ */
        /*             e->pos.x + dir.x * speed, */
        /*             e->pos.y + dir.y * speed, */
        /*         }}); */
        /* } */
    }
}

static void update_bullet(entity *e, f32 dt) {
    entity_set_pos(
        e,
        (vec2s) {{
            e->pos.x + e->bullet.velocity.x * dt,
            e->pos.y + e->bullet.velocity.y * dt,
        }});
}

static void update_turret(entity *e, f32 dt) {
    e->turret.angle += 0.05f;
}

static void draw_basic(entity *e) {
    ivec2s index = ENTITY_INFO[e->type].base_sprite;
    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ e->px.x, e->px.y }},
            .index = index,
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_LEVEL_ENTITY,
            .flags = GFX_NO_FLAGS
        });
}

static void draw_turret(entity *e) {
    draw_basic(e);

    const int nframes = 6;
    const int i =
        ((int) roundf((normalize_angle(e->turret.angle) + PI) / (TAU / nframes))) % nframes;
    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ e->tile.x * TILE_SIZE_PX, e->tile.y * TILE_SIZE_PX }},
            .index = {{
                0 + i, 6
            }},
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_LEVEL_ENTITY_OVERLAY,
            .flags = GFX_NO_FLAGS
        });
}

entity_info ENTITY_INFO[ENTITY_TYPE_COUNT] = {
    [ENTITY_TURRET_L0] = {
        .base_sprite = {{ 0, 5 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .update = update_turret,
        .unlock_price = 0,
        .buy_price = 25,
    },
    [ENTITY_TURRET_L1] = {
        .base_sprite = {{ 1, 5 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .update = update_turret,
        .unlock_price = 100,
        .buy_price = 50,
    },
    [ENTITY_TURRET_L2] = {
        .base_sprite = {{ 2, 5 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .update = update_turret,
        .unlock_price = 250,
        .buy_price = 100,
    },
    [ENTITY_BULLET_L0] = {
        .base_sprite = {{ 0, 7 }},
        .draw = draw_basic,
        .update = update_bullet,
    },
    [ENTITY_FLAG] = {
        .base_sprite = {{ 7, 1 }},
        .draw = draw_basic,
    },
    [ENTITY_TRUCK] = {
        .base_sprite = {{ 4, 1 }},
        .draw = draw_basic,
        .tick = tick_truck,
    },
};
