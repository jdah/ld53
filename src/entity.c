#include "entity.h"
#include "cjam/rand.h"
#include "direction.h"
#include "font.h"
#include "gfx.h"
#include "level.h"
#include "palette.h"
#include "sound.h"
#include "state.h"
#include "util.h"

#include <cjam/aabb.h>
#include <cjam/time.h>

#define ALIEN_BASE_DAMAGE_PER_SECOND 2.0f
#define ALIEN_BASE_SPEED 0.6f

static void play_hit_sound() {
    static struct rand r;
    if (!r.s[0]) {
        r = rand_create(0x12345);
    }

    char name[256];
    snprintf(name, sizeof(name), "hit_%d.wav", (int) rand_n(&r, 0, 2));
    sound_play(name, 1.0f);
}

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

aabb entity_aabb(const entity *e) {
    const entity_info *info = &ENTITY_INFO[e->type];
    return aabb_translate(info->aabb, VEC2S2I(e->pos));
}

ivec2s entity_center(const entity *e) {
    return aabb_center(entity_aabb(e));
}

static int find_collisions(const aabb *aabb, entity **entities, int n) {
    const ivec2s
        tmin = level_clamp_tile(glms_ivec2_add(level_px_to_tile(aabb->min), IVEC2S(-1))),
        tmax = level_clamp_tile(glms_ivec2_add(level_px_to_tile(aabb->max), IVEC2S(+1)));

    int i = 0;
    for (int x = tmin.x; x <= tmax.x; x++) {
        for (int y = tmin.y; y <= tmax.y; y++) {
            dlist_each(tile_node, &state->level->tile_entities[x][y], it) {
                const struct aabb_s b = entity_aabb(it.el);
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

static entity *shoot_bullet(entity_type type, vec2s origin, vec2s dir, ivec2s target) {
    entity_info *info = &ENTITY_INFO[type];
    entity *e = level_new_entity(state->level, type);
    e->bullet.velocity = glms_vec2_scale(dir, info->bullet.speed);
    entity_set_pos(e, origin);
    sound_play("shoot.wav", 0.25f);

    if (info->bullet.is_shell) {
        e->bullet.has_target = true;
        e->bullet.target = target;
    }

    return e;
}

// move e to point
static void entity_move_to_point(
    entity *e, vec2s goal, bool center, f32 speed, vec2s *move_out, direction *dir_out) {
    vec2s
        c = center ? IVEC2S2V(entity_center(e)) : e->pos,
        l = glms_vec2_sub(goal, c),
        m;

    bool move_x = false;
    if (fabsf(l.x - l.y) < 0.001f) {
        // prefer last direction
        move_x = fabsf(e->last_move.x) > fabsf(e->last_move.y);
    }

    m = (move_x || fabsf(l.x) > fabsf(l.y)) ?
        VEC2S(sign(l.x), 0.0f)
        : VEC2S(0.0f, sign(l.y));

    m = glms_vec2_scale(m, speed);

    // TODO: try_move
    entity_set_pos(e, glms_vec2_add(e->pos, m));

    if (move_out) {
        *move_out = m;
    }

    if (dir_out) {
        *dir_out = direction_from_vec2s(m, DIRECTION_DOWN);
    }
}

// move e on path, return true if hit target
static bool entity_move_on_path(
    entity *e, f32 speed, vec2s *move_out, direction *dir_out) {
    ASSERT(e->path);
    if (dynlist_size(e->path) == 1) {
        return true;
    }

    const ivec2s
        next = e->path[1],
        next_px = level_tile_center_px(next);

    entity_move_to_point(e, IVEC2S2V(next_px), true, speed, move_out, dir_out);
    return false;
}

static bool check_building_death(entity *e) {
    if (e->health < e->last_health && (state->time.tick % 5 == 0)) {
        play_hit_sound();
    }
    e->last_health = e->health;

    // TODO: sound
    const entity_info *info = &ENTITY_INFO[e->type];
    if (e->health <= 0) {
        e->delete = true;
        particle_new_multi_splat(
            IVEC2S2V(entity_center(e)),
            palette_get(info->palette),
            TICKS_PER_SECOND,
            5, 10, false);
        sound_play("explode.wav", 1.0f);
        return true;
    }

    return false;
}

static void explosion(vec2s pos, f32 radius, f32 damage) {
    const aabb area = AABB_CH(VEC2S2I(pos), IVEC2S(radius));
    entity *around[64];
    const int m =
        level_get_box_entities(
            state->level,
            &area,
            around,
            64);

    for (int i = 0; i < m; i++) {
        entity *f = around[i];
        if (E_INFO(f)->flags & EIF_ENEMY) {
            const f32 invdist =
                clamp(
                    1.0f - (glms_vec2_norm(glms_vec2_sub(f->pos, pos)) / radius),
                    0.0f, 1.0f);

            const f32 d = damage * (0.6f + invdist);
            f->health -= d;
            particle_new_multi_splat(
                IVEC2S2V(entity_center(f)),
                palette_get(E_INFO(f)->palette),
                10,
                2, 4,
                false);
            LOG("did %f damage", d);
        }
    }

    particle_new_multi_smoke(
        pos,
        palette_get(PALETTE_LIGHT_GRAY),
        35,
        10,
        20);
    particle_new_multi_splat(
        pos,
        palette_get(PALETTE_ORANGE),
        15,
        5,
        10,
        true);
}

static void tick_mine(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }

    entity *entities[64];
    const int n = level_get_tile_entities(state->level, e->tile, entities, 64);

    bool explode = false;

    for (int i = 0; i < n; i++) {
        if (E_INFO(entities[i])->flags & EIF_ENEMY) {
            explode = true;
            break;
        }
    }

    if (explode) {
        // explode
        e->delete = true;
        sound_play("mine.wav", 1.0f);

        const int mod = e->type - ENTITY_MINE_L0;
        explosion(
            IVEC2S2V(entity_center(e)),
            5 + (mod * 2),
            32.0f + (mod * 12.0f));
    }
}

static int truck_path_weight(const level *l, ivec2s p, void*) {
    if (!level_tile_in_bounds(p)) {
        return -1;
    }

    tile_type tile = l->tiles[p.x][p.y];
    if (tile != TILE_ROAD && tile != TILE_WAREHOUSE_FINISH) {
        return -1;
    }

    return 1;
}

int priority_turret_target(entity *e, entity *turret) {
    if (!(E_INFO(e)->flags & EIF_ENEMY)) { return -1; }

    if (E_INFO(e)->flags & EIF_SHIP) {
        return 1;
    }

    return 2;
}

static void tick_turret(entity *e) {
    if (state->stage != STAGE_PLAY) {
        e->turret.angle += 0.1f;
        return;
    }

    if (check_building_death(e)) { return; }

    const entity_info *info = &ENTITY_INFO[e->type];
    const f32 bpt = info->turret.bps / TICKS_PER_SECOND;
    const int t = state->time.tick + e->id.index;
    const int n_shoot = ((int) (floorf(t * bpt))) - ((int) (floorf((t - 1) * bpt)));

    entity *target = level_get_entity(state->level, e->turret.target);

    if (n_shoot > 0 && info->turret.is_cannon) {
        goto new_target;
    }

    if (target) {
        goto shoot;
    }

    // only find target every 5 ticks
    if ((e->id.index + state->time.tick) % 5) {
        goto shoot;
    }

new_target:
    target =
        level_find_nearest_entity(
            state->level, e->tile, (f_entity_priority) priority_turret_target, e);
    e->turret.target = target ? target->id : ENTITY_NONE;

shoot:
    if (!target) {
        // twist idly
        e->turret.angle += 0.1f;
        return;
    }

    const vec2s
        origin = (vec2s) {{ e->pos.x + 4, e->pos.y + 5 }},
        dir = glms_vec2_normalize(glms_vec2_sub(IVEC2S2V(entity_center(target)), origin));

    e->turret.angle = -atan2f(dir.y, dir.x);

    for (int i = 0; i < n_shoot; i++) {
        shoot_bullet(info->turret.bullet, origin, dir, target->tile);
    }
}

static void tick_basic_building(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }
    if (check_building_death(e)) { return; }
}

static void tick_truck(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }

    state->stats.truck_health = e->health;

    if (e->health < e->last_health && (state->time.tick % 5 == 0)) {
        play_hit_sound();
    }
    e->last_health = e->health;

    if (e->health <= 0) {
        sound_play("lose.wav", 1.0f);
        state_set_stage(state, STAGE_DONE);
    }

    if (!e->path) {
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

        e->truck.path_index = 0;
    }

    // move to next path point
    if (e->path) {
        if (state->time.tick % 5 == 0) {
            particle_new_smoke(
                IVEC2S2V(
                    glms_ivec2_add(entity_center(e), IVEC2S(1, 2))),
                palette_get(PALETTE_LIGHT_GRAY),
                35);
        }

        const ivec2s tile = e->path[e->truck.path_index];
        const ivec2s goal_px = level_tile_to_px(tile);

        const f32 speed = 0.35f + state->stats.truck_speed_level * 0.15f;
        if (glms_vec2_norm(glms_vec2_sub(e->pos, IVEC2S2V(goal_px))) < 1.5f) {
            e->truck.path_index++;
        }

        if (e->truck.path_index == dynlist_size(e->path)) {
            state_set_stage(state, STAGE_DONE);
        }

        entity_move_to_point(e, IVEC2S2V(goal_px), false, speed, &e->last_move, &e->truck.dir);
    }
}

static bool check_enemy_death(entity *e) {
    // TODO: sound
    const entity_info *info = &ENTITY_INFO[e->type];
    if (e->health <= 0) {
        e->delete = true;
        particle_new_text(
            IVEC2S2V(entity_center(e)),
            palette_get(PALETTE_YELLOW),
            TICKS_PER_SECOND,
            "+%d",
            info->enemy.bounty);
        particle_new_multi_splat(
            IVEC2S2V(entity_center(e)),
            palette_get(info->palette),
            TICKS_PER_SECOND,
            3, 5, false);
        state->stats.money += info->enemy.bounty;
        return true;
    }

    return false;
}

void tick_boombox(entity *e) {
    tick_basic_building(e);

    if (((e->id.index * 13) + state->time.tick) % 35 == 0) {
        particle_new_music(
            IVEC2S2V(entity_center(e)), 40);
    }
}

int priority_alien_target(entity *e, entity *alien) {
    const entity_info *info = E_INFO(e);
    if (e->type != ENTITY_TRUCK && !(info->flags & EIF_PLACEABLE)) {
        return -1;
    }

    f32 mod = 1.0f;
    if (e->type == ENTITY_TRUCK) {
        mod = 5.0f;
    } else if (e->type == ENTITY_DECOY_TRUCK) {
        mod = 3.0f;
    } else {
        // check for other aliens on tile, don't mob
        entity *entities[64];
        const int n =
            level_get_tile_entities(state->level, e->tile, entities, 64);
        for (int i = 0; i < n; i++) {
            if (E_INFO(entities[i])->flags & EIF_ENEMY) {
                mod = max(mod - 0.25f, 0.1f);
            }
        }
    }

    // higher priority when closer
    const f32 invdist =
        512.0f - glms_vec2_norm(glms_vec2_sub(e->pos, alien->pos));

    return mod * invdist;
}

static int alien_path_weight(const level *l, ivec2s p, void*) {
    if (!level_tile_in_bounds(p)) {
        return -1;
    }

    tile_type tile = l->tiles[p.x][p.y];
    int base = 1;

    switch (tile) {
    case TILE_MOUNTAIN: return -1;
    case TILE_LAKE: return -1;
    case TILE_SLUDGE: base = 10;
    case TILE_MARSH: base = 5;
    case TILE_STONE: base = 2;
    default:
    }

    return base + (l->music_level[p.x][p.y] * 10);
}

void tick_alien(entity *e) {
    e->last_move = VEC2S(0);

    if (state->stage != STAGE_PLAY) { return; }

    const entity_info *info = &ENTITY_INFO[e->type];
    if (check_enemy_death(e)) { return; }

    entity *target = level_get_entity(state->level, e->alien.target);
    if (target) { goto path; }

    target =
        level_find_nearest_entity(
            state->level, e->tile,
            (f_entity_priority) priority_alien_target,
            e);
    e->alien.target = target ? target->id : ENTITY_NONE;

    if (!target) { return; }

    // only path for aliens once every 5 ticks
path:
    if (e->path && ((e->id.index + state->time.tick) % 5) != 0) {
        goto move;
    }

    ASSERT(target);

    dynlist_free(e->path);
    const bool success =
        level_path(
            state->level,
            &e->path,
            e->tile,
            target->tile,
            alien_path_weight,
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

    const f32 speed = ALIEN_BASE_SPEED * info->enemy.speed;

    direction dir = DIRECTION_DOWN;

    if (entity_move_on_path(e, speed, &e->last_move, &dir)) {
       vec2s
            l = glms_vec2_sub(target->pos, e->pos),
            m = fabsf(l.x) > fabsf(l.y) ?
                VEC2S(sign(l.x), 0.0f)
                : VEC2S(0.0f, sign(l.y));

        m = glms_vec2_scale(m, speed);
        // TODO: try_move
        entity_set_pos(e, glms_vec2_add(e->pos, m));

        dir =
            direction_from_vec2s(
                glms_vec2_sub(target->pos, e->pos),
                DIRECTION_DOWN);

        const f32 dps = ALIEN_BASE_DAMAGE_PER_SECOND * info->enemy.strength;
        target->health -= dps / TICKS_PER_SECOND;
    }

    e->alien.dir = dir;
}

void tick_ship(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }

    const entity_info *info = E_INFO(e);

    if (check_enemy_death(e)) { return; }

    if (e->ticks_alive < TICKS_PER_SECOND) {
        if (e->ticks_alive == 0) {
            sound_play("ship.wav", 1.0f);
        }

        particle_new_fancy(
            IVEC2S2V(entity_center(e)),
            palette_get(PALETTE_LIGHT_BLUE),
            20);
    } else {
        for (int i = 0; i < (int) ARRLEN(info->ship.spawns); i++) {
            if (info->ship.spawns[i].spawn_type == ENTITY_TYPE_NONE) {
                break;
            }

            const f32 spt =
                info->ship.spawns[i].spawns_per_second / TICKS_PER_SECOND;
            const int t = (i * e->id.index * 13) + state->time.tick;
            const int n =
                ((int) (floorf(t * spt))) - ((int) (floorf((t - 1) * spt)));

            struct rand r = rand_create(state->time.tick + e->id.index);

            for (int j = 0; j < n; j++) {
                if (!rand_chance(&r, info->ship.spawns[i].chance)) {
                    continue;
                }

                const f32 a = rand_f64(&r, 0, TAU);
                entity *alien =
                    level_new_entity(state->level, info->ship.spawns[i].spawn_type);
                entity_set_pos(
                    alien,
                    glms_vec2_add(
                        e->pos, glms_vec2_scale(VEC2S(cos(a), sin(a)), 5.0f)));
                sound_play("spawn.wav", 1.0f);

            particle_new_fancy(
                IVEC2S2V(entity_center(alien)),
                palette_get(PALETTE_LIGHT_BLUE),
                4);
            }
        }
    }
}

void tick_bullet(entity *e) {
    if (state->stage != STAGE_PLAY) { return; }

    bool done = false;
    if (state->level->tiles[e->tile.x][e->tile.y] == TILE_MOUNTAIN) {
        done = true;
    }

    if (e->bullet.has_target) {
        if (done || glms_ivec2_eq(e->tile, e->bullet.target)) {
            e->delete = true;
            sound_play("mine.wav", 1.0f);

            const int mod = e->type - ENTITY_SHELL_L0;
            explosion(
                IVEC2S2V(entity_center(e)),
                6 + (mod * 2),
                16.0f + (mod * 8.0f));
        }
    } else {
        if (done) {
            e->delete = true;
        }

        const aabb box = entity_aabb(e);

        entity *entities[256];
        const int ncol = find_collisions(&box, entities, 256);

        for (int i = 0; i < ncol; i++) {
            entity *f = entities[i];
            const entity_info *f_info = &ENTITY_INFO[f->type];
            if (f_info->flags & EIF_ENEMY) {
                f->health -= 1.0f;
                e->delete = true;
                particle_new_splat(
                    IVEC2S2V(entity_center(f)),
                    palette_get(f_info->palette),
                    10);
                break;
            }
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

static void draw_ship(entity *e) {
    ivec2s index = ENTITY_INFO[e->type].base_sprite;
    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ e->px.x, e->px.y }},
            .index = index,
            .color = GRAYSCALE(1.0f, e->ticks_alive / (f32) TICKS_PER_SECOND),
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
    case DIRECTION_DOWN: offset = IVEC2S(3, 0); break;
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

    /* dynlist_each(e->path, it) { */
    /*     gfx_batcher_push_sprite( */
    /*         &state->batcher, */
    /*         &state->atlas.tile, */
    /*         &(gfx_sprite) { */
    /*             .index = {{ 0, 15 }}, */
    /*             .pos = {{ it.el->x * TILE_SIZE_PX, it.el->y * TILE_SIZE_PX }}, */
    /*             .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }}, */
    /*             .z = Z_UI, */
    /*             .flags = GFX_NO_FLAGS */
    /*         }); */
    /* } */
}

static void draw_health(entity *e) {
    if (state->stage == STAGE_BUILD) { return; }

    const int max = max(E_INFO(e)->max_health, 1);
    const f32 u = e->health / (f32) max;

    gfx_batcher_push_subimage(
        &state->batcher,
        state->atlas.tile.image,
        (vec2s) {{ e->tile.x * TILE_SIZE_PX, e->tile.y * TILE_SIZE_PX + 7 }},
        COLOR_WHITE,
        Z_LEVEL_ENTITY_OVERLAY - 0.002f,
        GFX_NO_FLAGS,
        (ivec2s) {{ 40, 0 }},
        (ivec2s) {{ 7, 1 }});

    gfx_batcher_push_subimage(
        &state->batcher,
        state->atlas.tile.image,
        (vec2s) {{ e->tile.x * TILE_SIZE_PX, e->tile.y * TILE_SIZE_PX + 7 }},
        COLOR_WHITE,
        Z_LEVEL_ENTITY_OVERLAY - 0.002f,
        GFX_NO_FLAGS,
        (ivec2s) {{ 32, 0 }},
        (ivec2s) {{ u * 7, 1 }});
}

static void draw_turret(entity *e) {
    draw_basic(e);
    draw_health(e);

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
    draw_health(e);

    if (glms_ivec2_eq(state->input.cursor.tile, e->tile)) {
        const int r = E_INFO(e)->radar.radius;
        for (int x = -r; x <= r; x++) {
            for (int y = -r; y <= r; y++) {
                const ivec2s offset = IVEC2S(x, y);

                gfx_batcher_push_sprite(
                    &state->batcher,
                    &state->atlas.tile,
                    &(gfx_sprite) {
                        .pos =
                            IVEC2S2V(
                                glms_ivec2_scale(
                                    glms_ivec2_add(e->tile, offset),
                                    TILE_SIZE_PX)),
                        .index = {{ 0, 14 }},
                        .color = GRAYSCALE(1.0f, 0.4f),
                        .z = Z_LEVEL_ENTITY_OVERLAY,
                        .flags = GFX_NO_FLAGS
                    });
            }
        }
    }

    if (state->stage == STAGE_BUILD) {
        // show potential alien spawns
        const int r = E_INFO(e)->radar.radius;
        for (int x = -r; x <= r; x++) {
            for (int y = -r; y <= r; y++) {
                const ivec2s offset = IVEC2S(x, y);
                const ivec2s tile = glms_ivec2_add(e->tile, offset);

                if (!level_tile_in_bounds(tile)) { continue; }

                if (!(state->level->flags[tile.x][tile.y] & LTF_ALIEN_SPAWN)) {
                    continue;
                }

                const vec4s col = palette_get(PALETTE_RED);
                font_str(
                    level_tile_to_px(tile),
                    Z_LEVEL_ENTITY_OVERLAY,
                    VEC4S(
                        col.r, col.g, col.b,
                        0.2f + 0.5f * fabsf(sinf(state->time.tick / 8.0f))),
                    FONT_DOUBLED,
                    "?");
            }
        }
    }

    // blinky bit
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
        glms_vec2_norm(e->last_move) > 0.0001f ?
            (((int) roundf(state->time.tick / (4 / info->enemy.speed))) % 2)
            : 0;
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

    vec4s color = palette_get(E_INFO(e)->palette);

    if (e->type == ENTITY_ALIEN_GHOST) {
        color.a = 0.6f;
    }

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = glms_vec2_add(e->pos, IVEC2S2V(sprite_offset)),
            .index = glms_ivec2_add(base, index_offset),
            .color = color,
            .z = Z_LEVEL_ENTITY_OVERLAY,
            .flags = flags
        });

    /* dynlist_each(e->path, it) { */
    /*     gfx_batcher_push_sprite( */
    /*         &state->batcher, */
    /*         &state->atlas.tile, */
    /*         &(gfx_sprite) { */
    /*             .index = {{ 0, 15 }}, */
    /*             .pos = {{ it.el->x * TILE_SIZE_PX, it.el->y * TILE_SIZE_PX }}, */
    /*             .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }}, */
    /*             .z = Z_UI, */
    /*             .flags = GFX_NO_FLAGS */
    /*         }); */
    /* } */
}

static bool can_place_basic(ivec2s tile) {
    if (!level_tile_in_bounds(tile)) { return false; }

    switch (state->level->tiles[tile.x][tile.y]) {
    case TILE_BASE: return true;
    default:
    }

    return false;

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
        .flags = EIF_PLACEABLE,
        .turret = {
            .bullet = ENTITY_BULLET_L0,
            .bps = 4.0f,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 7, 7 }}
        },
        .palette = PALETTE_L0,
        .max_health = 10,
        .base = {
            .health = 10,
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
        .flags = EIF_PLACEABLE,
        .turret = {
            .bullet = ENTITY_BULLET_L1,
            .bps = 7.5f,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 7, 7 }}
        },
        .palette = PALETTE_L1,
        .max_health = 20,
        .base = {
            .health = 20,
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
        .flags = EIF_PLACEABLE,
        .turret = {
            .bullet = ENTITY_BULLET_L2,
            .bps = 10.0f,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 6, 6 }}
        },
        .palette = PALETTE_L2,
        .max_health = 35,
        .base = {
            .health = 35,
        },
    },
    [ENTITY_CANNON_L0] = {
        .name = "CANNON MK. 1",
        .base_sprite = {{ 3, 7 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .unlock_price = 150,
        .buy_price = 75,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE,
        .turret = {
            .bullet = ENTITY_SHELL_L0,
            .bps = 0.7f,
            .is_cannon = true,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 6, 6 }}
        },
        .palette = PALETTE_BLACK,
        .max_health = 40,
        .base = {
            .health = 40,
        },
    },
    [ENTITY_CANNON_L1] = {
        .name = "CANNON MK. 2",
        .base_sprite = {{ 4, 7 }},
        .draw = draw_turret,
        .tick = tick_turret,
        .unlock_price = 500,
        .buy_price = 250,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE,
        .turret = {
            .bullet = ENTITY_SHELL_L1,
            .bps = 1.5f,
            .is_cannon = true,
        },
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 6, 6 }}
        },
        .palette = PALETTE_YELLOW,
        .max_health = 60,
        .base = {
            .health = 60,
        },
    },
    [ENTITY_MINE_L0] = {
        .name = "MINE MK. 1",
        .base_sprite = {{ 3, 4 }},
        .draw = draw_basic,
        .tick = tick_mine,
        .unlock_price = 100,
        .buy_price = 75,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE | EIF_CAN_SPAWN,
        .aabb = {
            .min = {{ 0, 2 }},
            .max = {{ 6, 4 }}
        },
        .palette = PALETTE_L0,
    },
    [ENTITY_MINE_L1] = {
        .name = "MINE MK. 2",
        .base_sprite = {{ 4, 4 }},
        .draw = draw_basic,
        .tick = tick_mine,
        .unlock_price = 200,
        .buy_price = 125,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE | EIF_CAN_SPAWN,
        .aabb = {
            .min = {{ 0, 2 }},
            .max = {{ 6, 4 }}
        },
        .palette = PALETTE_L1,
    },
    [ENTITY_MINE_L2] = {
        .name = "MINE MK. 3",
        .base_sprite = {{ 5, 4 }},
        .draw = draw_basic,
        .tick = tick_mine,
        .unlock_price = 400,
        .buy_price = 175,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE | EIF_CAN_SPAWN,
        .aabb = {
            .min = {{ 0, 2 }},
            .max = {{ 6, 4 }}
        },
        .palette = PALETTE_L2,
    },
    [ENTITY_RADAR_L0] = {
        .name = "RADAR MK. 1",
        .base_sprite = {{ 3, 5 }},
        .tick = tick_basic_building,
        .draw = draw_radar,
        .unlock_price = 150,
        .buy_price = 100,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE,
        .aabb = {
            .min = {{ 1, 0 }},
            .max = {{ 5, 5 }}
        },
        .palette = PALETTE_L0,
        .max_health = 5,
        .base = {
            .health = 5
        },
        .radar = {
            .radius = 1,
        },
    },
    [ENTITY_RADAR_L1] = {
        .name = "RADAR MK. 2",
        .base_sprite = {{ 4, 5 }},
        .tick = tick_basic_building,
        .draw = draw_radar,
        .unlock_price = 500,
        .buy_price = 250,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE,
        .aabb = {
            .min = {{ 1, 0 }},
            .max = {{ 5, 5 }}
        },
        .palette = PALETTE_L1,
        .max_health = 10,
        .base = {
            .health = 10
        },
        .radar = {
            .radius = 2,
        },
    },
    [ENTITY_DECOY_TRUCK] = {
        .name = "DECOY TRUCK",
        .base_sprite = {{ 6, 0 }},
        .tick = tick_basic_building,
        .draw = draw_basic,
        .unlock_price = 250,
        .buy_price = 150,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE,
        .aabb = {
            .min = {{ 1, 1 }},
            .max = {{ 7, 6 }}
        },
        .palette = 30,
        .max_health = 25,
        .base = {
            .health = 25
        },
    },
    [ENTITY_BOOMBOX] = {
        .name = "BOOMBOX",
        .base_sprite = {{ 3, 3 }},
        .tick = tick_boombox,
        .draw = draw_basic,
        .unlock_price = 750,
        .buy_price = 200,
        .can_place = can_place_basic,
        .flags = EIF_PLACEABLE,
        .aabb = {
            .min = {{ 1, 0 }},
            .max = {{ 5, 5 }}
        },
        .palette = PALETTE_BLACK,
        .max_health = 25,
        .base = {
            .health = 25
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
            .speed = 110.0f
        },
        .palette = 8,
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
            .speed = 140.0f
        },
        .palette = 8,
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
            .speed = 200.0f
        },
        .palette = 8,
    },
    [ENTITY_SHELL_L0] = {
        .base_sprite = {{ 2, 7 }},
        .draw = draw_basic,
        .tick = tick_bullet,
        .update = update_bullet,
        .flags = EIF_DOES_NOT_BLOCK,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 2, 2 }}
        },
        .bullet = {
            .speed = 32.0f,
            .is_shell = true
        },
        .palette = 8,
    },
    [ENTITY_SHELL_L1] = {
        .base_sprite = {{ 2, 7 }},
        .draw = draw_basic,
        .tick = tick_bullet,
        .update = update_bullet,
        .flags = EIF_DOES_NOT_BLOCK,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 2, 2 }}
        },
        .bullet = {
            .speed = 40.0f,
            .is_shell = true
        },
        .palette = 8,
    },
    [ENTITY_FLAG] = {
        .base_sprite = {{ 7, 1 }},
        .draw = draw_basic,

    },
    [ENTITY_TRUCK] = {
        .base_sprite = {{ 3, 1 }},
        .draw = draw_truck,
        .tick = tick_truck,
        .aabb = {
            .min = {{ 1, 1 }},
            .max = {{ 7, 6 }}
        },
        .base = {
            .health = 10
        }
    },
    [ENTITY_ALIEN_L0] = {
        .base_sprite = {{ 0, 10 }},
        .draw = draw_alien,
        .tick = tick_alien,
        .flags = EIF_ENEMY | EIF_ALIEN,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 4, 4 }}
        },
        .palette = PALETTE_ALIEN_GREEN,
        .enemy = {
            .speed = 1.0f,
            .strength = 1.0f,
            .bounty = 5
        },
        .base = {
            .health = 10
        },
    },
    [ENTITY_ALIEN_L1] = {
        .base_sprite = {{ 0, 10 }},
        .draw = draw_alien,
        .tick = tick_alien,
        .flags = EIF_ENEMY | EIF_ALIEN,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 4, 4 }}
        },
        .palette = PALETTE_ALIEN_RED,
        .enemy = {
            .speed = 1.2f,
            .strength = 1.2f,
            .bounty = 15
        },
        .base = {
            .health = 15
        },
    },
    [ENTITY_ALIEN_L2] = {
        .base_sprite = {{ 0, 10 }},
        .draw = draw_alien,
        .tick = tick_alien,
        .flags = EIF_ENEMY | EIF_ALIEN,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 4, 4 }}
        },
        .palette = PALETTE_ALIEN_BLUE,
        .enemy = {
            .speed = 1.4f,
            .strength = 1.4f,
            .bounty = 25
        },
        .base = {
            .health = 25
        },
    },
    [ENTITY_ALIEN_FAST] = {
        .base_sprite = {{ 0, 10 }},
        .draw = draw_alien,
        .tick = tick_alien,
        .flags = EIF_ENEMY | EIF_ALIEN,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 4, 4 }}
        },
        .palette = PALETTE_ALIEN_YELLOW,
        .enemy = {
            .speed = 2.2f,
            .strength = 0.9f,
            .bounty = 50
        },
        .base = {
            .health = 10
        },
    },
    [ENTITY_ALIEN_TANK] = {
        .base_sprite = {{ 0, 10 }},
        .draw = draw_alien,
        .tick = tick_alien,
        .flags = EIF_ENEMY | EIF_ALIEN,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 4, 4 }}
        },
        .palette = PALETTE_ALIEN_GREY,
        .enemy = {
            .speed = 0.5f,
            .strength = 2.5f,
            .bounty = 100
        },
        .base = {
            .health = 50
        },
    },
    [ENTITY_ALIEN_GHOST] = {
        .base_sprite = {{ 0, 10 }},
        .draw = draw_alien,
        .tick = tick_alien,
        .flags = EIF_ENEMY | EIF_ALIEN,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 4, 4 }}
        },
        .palette = PALETTE_ALIEN_GREY,
        .enemy = {
            .speed = 1.4f,
            .strength = 1.2f,
            .bounty = 100
        },
        .base = {
            .health = 20
        },
    },
    [ENTITY_SHIP_L0] = {
        .base_sprite = {{ 0, 8 }},
        .draw = draw_ship,
        .tick = tick_ship,
        .flags = EIF_ENEMY | EIF_SHIP,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 7, 5 }}
        },
        .palette = PALETTE_ALIEN_GREY,
        .enemy = {
            .strength = 1.0f,
            .bounty = 10
        },
        .ship = {
            .spawns = {
                { ENTITY_ALIEN_L0, 0.5f, 1.0f },
                { ENTITY_ALIEN_L1, 0.025f, 0.1f },
            }
        },
        .base = {
            .health = 25
        },
    },
    [ENTITY_SHIP_L1] = {
        .base_sprite = {{ 1, 8 }},
        .draw = draw_ship,
        .tick = tick_ship,
        .flags = EIF_ENEMY | EIF_SHIP,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 7, 5 }}
        },
        .palette = PALETTE_ALIEN_RED,
        .enemy = {
            .strength = 1.0f,
            .bounty = 25
        },
        .ship = {
            .spawns = {
                { ENTITY_ALIEN_L1, 0.5f, 0.9f },
                { ENTITY_ALIEN_L1, 0.25f, 0.85f },
                { ENTITY_ALIEN_TANK, 0.025f, 0.05f },
            }
        },
        .base = {
            .health = 35
        },
    },
    [ENTITY_SHIP_L2] = {
        .base_sprite = {{ 1, 8 }},
        .draw = draw_ship,
        .tick = tick_ship,
        .flags = EIF_ENEMY | EIF_SHIP,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 7, 5 }}
        },
        .palette = PALETTE_ALIEN_GREEN,
        .enemy = {
            .strength = 1.0f,
            .bounty = 50
        },
        .ship = {
            .spawns = {
                { ENTITY_ALIEN_L0, 0.25f, 0.5f },
                { ENTITY_ALIEN_L1, 0.25f, 0.95f },
                { ENTITY_ALIEN_L2, 0.25f, 0.95f },
                { ENTITY_ALIEN_TANK, 0.025f, 0.07f },
                { ENTITY_ALIEN_FAST, 0.025f, 0.07f },
                { ENTITY_ALIEN_GHOST, 0.025f, 0.07f },
            }
        },
        .base = {
            .health = 50
        },
    },
    [ENTITY_TRANSPORT_L0] = {
        .base_sprite = {{ 0, 9 }},
        .draw = draw_ship,
        .tick = tick_ship,
        .flags = EIF_ENEMY | EIF_SHIP,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 6, 6 }}
        },
        .palette = PALETTE_ALIEN_GREY,
        .enemy = {
            .strength = 1.0f,
            .bounty = 100
        },
        .ship = {
            .spawns = {
                { ENTITY_ALIEN_L0, 1.0f, 0.6f },
                { ENTITY_ALIEN_L1, 0.1f, 0.2f },
                { ENTITY_ALIEN_TANK, 0.25f, 0.2f },
                { ENTITY_ALIEN_GHOST, 0.25f, 0.2f },
            }
        },
        .base = {
            .health = 50
        },
    },
    [ENTITY_TRANSPORT_L1] = {
        .base_sprite = {{ 1, 9 }},
        .draw = draw_ship,
        .tick = tick_ship,
        .flags = EIF_ENEMY | EIF_SHIP,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 6, 6 }}
        },
        .palette = PALETTE_ALIEN_RED,
        .enemy = {
            .strength = 1.0f,
            .bounty = 250
        },
        .ship = {
            .spawns = {
                { ENTITY_ALIEN_L1, 1.0f, 0.7f },
                { ENTITY_ALIEN_L1, 0.5f, 0.85f },
                { ENTITY_ALIEN_TANK, 0.25f, 0.5f },
                { ENTITY_ALIEN_FAST, 0.25f, 0.5f },
                { ENTITY_ALIEN_GHOST, 0.25f, 0.5f },
            }
        },
        .base = {
            .health = 80
        },
    },
    [ENTITY_TRANSPORT_L2] = {
        .base_sprite = {{ 2, 9 }},
        .draw = draw_ship,
        .tick = tick_ship,
        .flags = EIF_ENEMY | EIF_SHIP,
        .aabb = {
            .min = {{ 0, 0 }},
            .max = {{ 6, 6 }}
        },
        .palette = PALETTE_ALIEN_GREEN,
        .enemy = {
            .strength = 1.0f,
            .bounty = 1000
        },
        .ship = {
            .spawns = {
                { ENTITY_ALIEN_L0, 3.0f, 0.9f },
                { ENTITY_ALIEN_L1, 2.0f, 0.85f },
                { ENTITY_ALIEN_TANK, 0.25f, 0.5f },
                { ENTITY_ALIEN_FAST, 0.25f, 0.5f },
                { ENTITY_ALIEN_GHOST, 0.25f, 0.5f },
            }
        },
        .base = {
            .health = 100
        },
    },
    [ENTITY_REPAIR] = {
        .name = "REPAIR 10",
        .base_sprite = {{ 7, 0 }},
        .unlock_price = 0,
        .buy_price = 50,
        .flags = EIF_NOT_AN_ENTITY
    },
    [ENTITY_ARMOR_UPGRADE] = {
        .name = "ARMOR UPGRADE",
        .base_sprite = {{ 8, 0 }},
        .unlock_price = 0,
        .buy_price = 1000,
        .flags = EIF_NOT_AN_ENTITY
    },
    [ENTITY_SPEED_UPGRADE] = {
        .name = "SPEED UPGRADE",
        .base_sprite = {{ 9, 0 }},
        .unlock_price = 0,
        .buy_price = 1000,
        .flags = EIF_NOT_AN_ENTITY
    }
};
