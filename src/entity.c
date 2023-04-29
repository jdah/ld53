#include "entity.h"
#include "direction.h"
#include "gfx.h"
#include "level.h"
#include "state.h"
#include "util.h"

#include <cjam/aabb.h>
#include <cjam/time.h>

#define ALIEN_BASE_DAMAGE_PER_SECOND 1.0f
#define ALIEN_BASE_SPEED 0.6f

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

static aabb aabb_for_entity(const entity *e) {
    const entity_info *info = &ENTITY_INFO[e->type];
    return aabb_translate(info->aabb, VEC2S2I(e->pos));
}

static int find_collisions(const aabb *aabb, entity **entities, int n) {
    const ivec2s
        tmin = level_clamp_tile(glms_ivec2_add(level_px_to_tile(aabb->min), IVEC2S(-1))),
        tmax = level_clamp_tile(glms_ivec2_add(level_px_to_tile(aabb->max), IVEC2S(+1)));

    int i = 0;
    for (int x = tmin.x; x <= tmax.x; x++) {
        for (int y = tmin.y; y <= tmax.y; y++) {
            dlist_each(tile_node, &state->level->tile_entities[x][y], it) {
                const struct aabb_s b = aabb_for_entity(it.el);
                if (aabb_collides(*aabb, b)) {
                    entities[i++] = it.el;
                    if (i == n) {
                        WARN("ran out of space for collisions");
                        return n;
                    }
                }
            }
        }
    }

    return i;
}

static entity *shoot_bullet(entity_type type, vec2s origin, vec2s dir) {
    entity_info *info = &ENTITY_INFO[type];
    entity *e = level_new_entity(state->level, type);
    e->bullet.velocity = glms_vec2_scale(dir, info->bullet.speed);
    entity_set_pos(e, origin);
    return e;
}

// move e on path, return true if hit target
static bool entity_move_on_path(entity *e, f32 speed, bool center, direction *dir_out) {
    ASSERT(e->path);
    if (dynlist_size(e->path) == 1) {
        return true;
    }

    const ivec2s
        next = e->path[1],
        next_px = center ? level_tile_center_px(next) : level_tile_to_px(next);

    vec2s
        l = glms_vec2_sub(IVEC2S2V(next_px), e->pos),
        m = fabsf(l.x) > fabsf(l.y) ?
            VEC2S(sign(l.x), 0.0f)
            : VEC2S(0.0f, sign(l.y));

    m = glms_vec2_scale(m, speed);

    // TODO: try_move
    entity_set_pos(e, glms_vec2_add(e->pos, m));

    if (dir_out) {
        *dir_out = direction_from_vec2s(m, DIRECTION_DOWN);
    }

    return false;
}

int truck_path_weight(const level *l, ivec2s p, void*) {
    if (!level_tile_in_bounds(p)) {
        return -1;
    }

    tile_type tile = l->tiles[p.x][p.y];
    if (tile != TILE_ROAD && tile != TILE_WAREHOUSE_FINISH) {
        return -1;
    }

    return 1;
}

bool filter_turret_target(entity *e) {
    return e->type == ENTITY_ALIEN_L0;
}

static void tick_turret(entity *e) {
    if (state->stage != STAGE_PLAY) {
        e->turret.angle += 0.1f;
        return;
    }

    entity *target = level_get_entity(state->level, e->turret.target);

    if (target) {
        goto shoot;
    }

    // only find target every 5 ticks
    if ((e->id.index + state->time.tick) % 5) {
        goto shoot;
    }

    target =
        level_find_nearest_entity(state->level, e->tile, filter_turret_target);
    e->turret.target = target ? target->id : ENTITY_NONE;

shoot:
    if (!target) {
        // twist idly
        e->turret.angle += 0.1f;

        return;
    }

    const entity_info *info = &ENTITY_INFO[e->type];
    const f32 bpt = info->turret.bps / TICKS_PER_SECOND;
    const int n =
        ((int) (floorf(state->time.tick * bpt)))
            - ((int) (floorf((state->time.tick - 1) * bpt)));

    const vec2s
        origin = (vec2s) {{ e->pos.x + 4, e->pos.y + 5 }},
        dir = glms_vec2_normalize(glms_vec2_sub(target->pos, origin));

    e->turret.angle = -atan2f(dir.y, dir.x);

    for (int i = 0; i < n; i++) {
        shoot_bullet(info->turret.bullet, origin, dir);
    }
}

static void tick_truck(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }

    dynlist_free(e->path);

    const bool success =
        level_path(
            state->level,
            &e->path,
            e->tile,
            state->level->finish,
            truck_path_weight,
            NULL);

    if (!success) {
        dynlist_free(e->path);
        e->path = NULL;
    }

    // move to next path point
    if (success) {
        if (entity_move_on_path(e, 0.5f, false, &e->truck.dir)) {
            state_set_stage(state, STAGE_DONE);
        }
    }
}

void tick_alien(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }

    // TODO: splatter
    if (e->alien.health <= 0) {
        e->delete = true;
        return;
    }

    // only path for aliens once every 5 ticks
    if (e->path && ((e->id.index + state->time.tick) % 5) != 0) {
        goto move;
    }

    entity *truck = level_find_entity(state->level, ENTITY_TRUCK);
    ASSERT(truck);

    dynlist_free(e->path);
    const bool success =
        level_path(
            state->level,
            &e->path,
            e->tile,
            truck->tile,
            level_path_default_weight,
            NULL);

    if (!success) {
        dynlist_free(e->path);
        e->path = NULL;
    }

move:
    if (!e->path) {
        WARN("alien %d has no path", e->id.index);
        return;
    }

    const entity_info *info = &ENTITY_INFO[e->type];
    const f32 speed = ALIEN_BASE_SPEED * info->alien.speed;

    direction dir = DIRECTION_DOWN;

    if (entity_move_on_path(e, speed, true, &dir)
        && ((e->id.index + state->time.tick) % TICKS_PER_SECOND) == 0) {
        dir =
            direction_from_vec2s(
                glms_vec2_sub(truck->pos, e->pos),
                DIRECTION_DOWN);
    }

    // try to hit truck
    // TODO: dps?
    if (((e->id.index + state->time.tick) % TICKS_PER_SECOND) == 0) {
        entity *entities[256];
        const aabb box = aabb_for_entity(e);
        const int ncol = find_collisions(&box, entities, 256);
        for (int i = 0; i < ncol; i++) {
            if (entities[i]->type == ENTITY_TRUCK) {
                state->stats.health -=
                    ALIEN_BASE_DAMAGE_PER_SECOND * info->alien.strength;
                break;
            }
        }
    }

    e->alien.dir = dir;
}

void tick_bullet(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }

    const aabb box = aabb_for_entity(e);

    entity *entities[256];
    const int ncol = find_collisions(&box, entities, 256);

    for (int i = 0; i < ncol; i++) {
        entity *f = entities[i];
        const entity_info *f_info = &ENTITY_INFO[f->type];
        if (f_info->flags & EIF_ALIEN) {
            f->alien.health -= 1.0f;
            e->delete = true;
            break;
        }
    }
}

static void update_bullet(entity *e, f32 dt) {
    if (state->stage != STAGE_PLAY) { return; }

    entity_set_pos(
        e,
        (vec2s) {{
            e->pos.x + e->bullet.velocity.x * dt,
            e->pos.y + e->bullet.velocity.y * dt,
        }});
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

static void draw_truck(entity *e) {
    const ivec2s index = ENTITY_INFO[e->type].base_sprite;

    ivec2s offset;
    switch (e->truck.dir) {
    case DIRECTION_LEFT: offset = IVEC2S(0, 0); break;
    case DIRECTION_RIGHT: offset = IVEC2S(1, 0); break;
    case DIRECTION_UP: offset = IVEC2S(2, 0); break;
    case DIRECTION_DOWN: offset = IVEC2S(0, 0); break;
    default: ASSERT(false);
    }

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ e->px.x, e->px.y }},
            .index = glms_ivec2_add(index, offset),
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_LEVEL_ENTITY,
            .flags = GFX_NO_FLAGS
        });
}

static void draw_turret(entity *e) {
    draw_basic(e);

    const int nframes = 6;
    const int i =
        ((int) roundf(wrap_angle(e->turret.angle + PI) / (TAU / nframes))) % nframes;
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

static void draw_radar(entity *e) {
    draw_basic(e);

    if ((((e->id.index * 17) + state->time.tick / 5)) % 6 == 0) { return; }

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ e->tile.x * TILE_SIZE_PX, e->tile.y * TILE_SIZE_PX }},
            .index = {{ 5, 5 }},
            .color = COLOR_WHITE,
            .z = Z_LEVEL_ENTITY_OVERLAY,
            .flags = GFX_NO_FLAGS
        });
}

static void draw_alien(entity *e) {
    entity_info *info = &ENTITY_INFO[e->type];
    const ivec2s base = info->base_sprite;
    const int i =
        ((int) roundf(state->time.tick / (4 / info->alien.speed))) % 2;
    ivec2s index_offset, sprite_offset = (ivec2s) {{ 0, 0 }};
    int flags = GFX_NO_FLAGS;

    switch (e->alien.dir) {
    case DIRECTION_UP:
        index_offset = (ivec2s) {{ 0, 1 }};
        sprite_offset = (ivec2s) {{ -1, 0 }};
        if (i) {
            sprite_offset = (ivec2s) {{ -2, 0 }};
            flags |= GFX_FLIP_X;
        }
        break;
    case DIRECTION_DOWN:
        index_offset = (ivec2s) {{ i, 0 }};
        break;
    case DIRECTION_RIGHT:
        index_offset = (ivec2s) {{ i, 2 }};
        break;
    case DIRECTION_LEFT:
        index_offset = (ivec2s) {{ i, 2 }};
        sprite_offset = (ivec2s) {{ -3, 0 }};
        flags |= GFX_FLIP_X;
        break;
    default: ASSERT(false);
    }

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = glms_vec2_add(e->pos, IVEC2S2V(sprite_offset)),
            .index = glms_ivec2_add(base, index_offset),
            .color = COLOR_WHITE,
            .z = Z_LEVEL_ENTITY_OVERLAY,
            .flags = flags
        });

    dynlist_each(e->path, it) {
        gfx_batcher_push_sprite(
            &state->batcher,
            &state->atlas.tile,
            &(gfx_sprite) {
                .index = {{ 0, 15 }},
                .pos = {{ it.el->x * TILE_SIZE_PX, it.el->y * TILE_SIZE_PX }},
                .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
                .z = Z_UI,
                .flags = GFX_NO_FLAGS
            });
    }
}

static bool can_place_basic(ivec2s tile) {
    if (!level_tile_in_bounds(tile)) { return false; }

    dlist_each(tile_node, &state->level->tile_entities[tile.x][tile.y], it) {
        const int flags = ENTITY_INFO[it.el->type].flags;
        if (!(flags & EIF_DOES_NOT_BLOCK)) {
            return false;
        }
    }

    return true;
}

entity_info ENTITY_INFO[ENTITY_TYPE_COUNT] = {
    [ENTITY_TURRET_L0] = {
        .name = "TURRET MK. 1",
        .base_sprite = {{ 0, 5 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .unlock_price = 0,
        .buy_price = 25,
        .can_place = can_place_basic,
        .turret = {
            .bullet = ENTITY_BULLET_L0,
            .bps = 4.0f,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 7, 7 }}
        },
    },
    [ENTITY_TURRET_L1] = {
        .name = "TURRET MK. 2",
        .base_sprite = {{ 1, 5 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .unlock_price = 100,
        .buy_price = 50,
        .can_place = can_place_basic,
        .turret = {
            .bullet = ENTITY_BULLET_L1,
            .bps = 7.5f,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 7, 7 }}
        },
    },
    [ENTITY_TURRET_L2] = {
        .name = "TURRET MK. 3",
        .base_sprite = {{ 2, 5 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .unlock_price = 250,
        .buy_price = 100,
        .can_place = can_place_basic,
        .turret = {
            .bullet = ENTITY_BULLET_L2,
            .bps = 10.0f,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 6, 6 }}
        },
    },
    [ENTITY_MINE_L0] = {
        .name = "MINE MK. 1",
        .base_sprite = {{ 3, 4 }},
        .draw = draw_basic,
        .unlock_price = 100,
        .buy_price = 75,
        .can_place = can_place_basic,
        .aabb = {
            .min = {{ 0, 2 }},
            .max = {{ 6, 4 }}
        },
    },
    [ENTITY_MINE_L1] = {
        .name = "MINE MK. 2",
        .base_sprite = {{ 4, 4 }},
        .draw = draw_basic,
        .unlock_price = 200,
        .buy_price = 125,
        .can_place = can_place_basic,
        .aabb = {
            .min = {{ 0, 2 }},
            .max = {{ 6, 4 }}
        },
    },
    [ENTITY_MINE_L2] = {
        .name = "MINE MK. 3",
        .base_sprite = {{ 5, 4 }},
        .draw = draw_basic,
        .unlock_price = 400,
        .buy_price = 175,
        .can_place = can_place_basic,
        .aabb = {
            .min = {{ 0, 2 }},
            .max = {{ 6, 4 }}
        },
    },
    [ENTITY_RADAR_L0] = {
        .name = "RADAR MK. 1",
        .base_sprite = {{ 3, 5 }},
        .draw = draw_radar,
        .unlock_price = 150,
        .buy_price = 100,
        .can_place = can_place_basic,
        .aabb = {
            .min = {{ 1, 0 }},
            .max = {{ 5, 5 }}
        },
    },
    [ENTITY_RADAR_L1] = {
        .name = "RADAR MK. 2",
        .base_sprite = {{ 4, 5 }},
        .draw = draw_radar,
        .unlock_price = 500,
        .buy_price = 250,
        .can_place = can_place_basic,
        .aabb = {
            .min = {{ 1, 0 }},
            .max = {{ 5, 5 }}
        },
    },
    [ENTITY_BOOMBOX] = {
        .name = "BOOMBOX",
        .base_sprite = {{ 3, 3 }},
        .draw = draw_basic,
        .unlock_price = 750,
        .buy_price = 200,
        .can_place = can_place_basic,
        .aabb = {
            .min = {{ 1, 0 }},
            .max = {{ 5, 5 }}
        },
    },
    [ENTITY_BULLET_L0] = {
        .base_sprite = {{ 0, 7 }},
        .draw = draw_basic,
        .tick = tick_bullet,
        .update = update_bullet,
        .flags = EIF_DOES_NOT_BLOCK,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 1, 1 }}
        },
        .bullet = {
            .speed = 80.0f
        },
    },
    [ENTITY_BULLET_L1] = {
        .base_sprite = {{ 0, 7 }},
        .draw = draw_basic,
        .tick = tick_bullet,
        .update = update_bullet,
        .flags = EIF_DOES_NOT_BLOCK,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 1, 1 }}
        },
        .bullet = {
            .speed = 100.0f
        },
    },
    [ENTITY_BULLET_L2] = {
        .base_sprite = {{ 0, 7 }},
        .draw = draw_basic,
        .tick = tick_bullet,
        .update = update_bullet,
        .flags = EIF_DOES_NOT_BLOCK,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 1, 1 }}
        },
        .bullet = {
            .speed = 150.0f
        },
    },
    [ENTITY_FLAG] = {
        .base_sprite = {{ 7, 1 }},
        .draw = draw_basic,

    },
    [ENTITY_TRUCK] = {
        .base_sprite = {{ 3, 1 }},
        .draw = draw_truck,
        .tick = tick_truck,
    },
    [ENTITY_ALIEN_L0] = {
        .base_sprite = {{ 0, 10 }},
        .draw = draw_alien,
        .tick = tick_alien,
        .flags = EIF_ALIEN,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 4, 4 }}
        },
        .alien = {
            .speed = 1.0f,
            .strength = 1.0f,
        },
        .base = {
            .alien = { .health = 10 }
        },
    },
};
