#include "level.h"
#include "defs.h"
#include "state.h"
#include "entity.h"
#include "direction.h"
#include "util.h"

#include <cjam/time.h>
#include <cjam/rand.h>
#include <cjam/map.h>

const char char_to_tile[256] = {
    ['?'] = TILE_NONE,
    [' '] = TILE_BASE,
    ['r'] = TILE_ROAD,
    ['S'] = TILE_ROAD,
    ['F'] = TILE_ROAD,
};

const char char_to_entity[256] = {
    ['S'] = ENTITY_START_POINT,
    ['F'] = ENTITY_FLAG,
};

const char *map[LEVEL_HEIGHT] = {
    "              F    ",
    "              r    ",
    "              r    ",
    "              r    ",
    "        rrrrrrr    ",
    "        r          ",
    "        r          ",
    "Srrrrrrrr          ",
    "                   ",
    "                   ",
    "                   ",
    "                   ",
    "                   ",
};

void level_init(level *level) {
    memset(level->tiles, 0, sizeof(level->tiles));

    // TODO: free
    level->entities = calloc(1, MAX_ENTITIES * sizeof(entity));

    for (int x = 0; x < LEVEL_WIDTH; x++) {
        for (int y = 0; y < LEVEL_HEIGHT; y++) {
            const char c = map[LEVEL_HEIGHT - y - 1][x];
            level->tiles[x][y] = char_to_tile[(int) c];

            entity_type etype = char_to_entity[(int) c];
            if (etype != ENTITY_TYPE_NONE) {
                entity *e = level_new_entity(level);
                e->type = etype;
                entity_set_pos(
                    e,
                    IVEC2S2V(level_tile_to_px((ivec2s) {{ x, y }})));
            }
        }
    }

    entity *start = level_find_entity(level, ENTITY_START_POINT);
    ASSERT(start, "level has no start");

    entity *truck = level_new_entity(level);
    truck->type = ENTITY_TRUCK;
    entity_set_pos(truck, IVEC2S2V(level_tile_to_px(start->tile)));
}

void level_tick(level *level) {
    DYNLIST(entity*) delete_entities = NULL;

    dlist_each(node, &level->all_entities, it) {
        ASSERT(!it.el->delete);

        f_entity_tick f_tick = ENTITY_INFO[it.el->type].tick;
        if (f_tick) { f_tick(it.el); }

        if (!level_px_in_bounds(it.el->px)
            || !level_tile_in_bounds(it.el->tile)) {
            it.el->delete = true;
        }

        if (it.el->delete) {
            *dynlist_push(delete_entities) = it.el;
        }
    }

    dynlist_each(delete_entities, it) {
        level_delete_entity(level, *it.el);
    }

    dynlist_free(delete_entities);
}

void level_update(level *level, f32 dt) {
    dlist_each(node, &level->all_entities, it) {
        f_entity_update f_update = ENTITY_INFO[it.el->type].update;
        if (f_update) { f_update(it.el, dt); }
    }
}

static void tile_draw(
    const level *level, ivec2s lpos, tile_type tile) {
    ivec2s index = {{ 0, 0 }};
    f32 z = Z_LEVEL_BASE;
    struct rand rng = rand_create(lpos.x * (lpos.y * 13));

    switch (tile) {
    case TILE_COUNT: ASSERT(false);
    case TILE_NONE: return;
    case TILE_BASE: {
        index = (ivec2s) {{
            0 + rand_n(&rng, 0, 3),
            0,
        }};
    } break;
    case TILE_ROAD: {
        tile_draw(level, lpos, TILE_BASE);

        bool surround[3][3];
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                const ivec2s pos = {{ lpos.x + x, lpos.y + y }};
                if (level_tile_in_bounds(pos)) {
                    surround[x + 1][y + 1] =
                        level->tiles[pos.x][pos.y] == TILE_ROAD;
                } else {
                    surround[x + 1][y + 1] = true;
                }
            }
        }

        ivec2s offset;
        if (surround[1][0] && surround[2][1]) {
            // top left corner
            offset = (ivec2s) {{ 0, 2 }};
        } else if (surround[0][1] && surround[1][0]) {
            // top right corner
            offset = (ivec2s) {{ 2, 2 }};
        } else if (surround[1][2] && surround[2][1]) {
            // bottom left corner
            offset = (ivec2s) {{ 0, 0 }};
        } else if (surround[1][2] && surround[0][1]) {
            // bottom right corner
            offset = (ivec2s) {{ 2, 0 }};
        } else if (surround[1][0] && surround[1][2]) {
            // vertical
            offset = (ivec2s) {{ 0, 1 }};
        } else if (surround[0][1] && surround[2][1]) {
            // horizontal
            offset = (ivec2s) {{ 1, 0 }};
        }

        index = (ivec2s) {{
            0 + offset.x,
            1 + offset.y
        }};
        z = Z_LEVEL_OVERLAY;
    } break;
    }

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ lpos.x * TILE_SIZE_PX, lpos.y * TILE_SIZE_PX }},
            .index = index,
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = z,
            .flags = GFX_NO_FLAGS
        });
}

void level_draw(const level *level) {
    for (int x = 0; x < LEVEL_WIDTH; x++) {
        for (int y = 0; y < LEVEL_HEIGHT; y++) {
            tile_draw(level, (ivec2s) {{ x, y }}, level->tiles[x][y]);
        }
    }

    dlist_each(node, &level->all_entities, it) {
        f_entity_draw f_draw = ENTITY_INFO[it.el->type].draw;
        if (f_draw) { f_draw(it.el); }
    }
}

entity *level_new_entity(level *level) {
    int n = 0, i = level->last_free_entity;
    while (n < MAX_ENTITIES) {
        if (!level->entities[i].id.present) {
            break;
        }

        i = (i + 1) % MAX_ENTITIES;
    }

    level->last_free_entity = i;
    entity *e = &level->entities[i];
    const u16 old_gen = e->id.gen;
    memset(e, 0, sizeof(*e));
    e->id = (entity_id) {
        .present = true,
        .gen = old_gen + 1,
        .index = i
    };
    dlist_append(node, &level->all_entities, e);
    return e;
}

void level_delete_entity(level *level, entity *e) {
    dlist_remove(node, &level->all_entities, e);

    if (e->on_tile) {
        dlist_remove(
            tile_node,
            &state->level->tile_entities[e->tile.x][e->tile.y],
            e);
    }

    e->id.present = false;
}

entity *level_get_entity(level *level, entity_id id) {
    if (!memcmp(
            &level->entities[id.index].id,
            &id,
            sizeof(id))) {
        return &level->entities[id.index];
    }

    return NULL;
}

entity *level_find_entity(level *l, entity_type type) {
    dlist_each(node, &l->all_entities, it) {
        if (it.el->type == type) {
            return it.el;
        }
    }

    return NULL;
}

static hash_type hash_ivec2s(ivec2s v) {
    return ((hash_type) (v.x) << 32) | ((hash_type) (v.y));
}

static int cmp_ivec2s(ivec2s a, ivec2s b) {
    return memcmp(&a, &b, sizeof(a));
}

static void *ivec2s_to_voidp(ivec2s v) {
    v.x &= 0xFFFF;
    v.y &= 0xFFFF;
    return (void*) ((((uintptr_t) v.x << 16) | ((uintptr_t) v.y)));
}

static ivec2s voidp_to_ivec2s(void *p) {
    uintptr_t v = (uintptr_t) p;
    return (ivec2s) {{ (v >> 16) & 0xFFFF, (v >> 0) & 0xFFFF }};
}

static hash_type level_path__hash(void *p, void*) {
    return hash_ivec2s(voidp_to_ivec2s(p));
}

static int level_path__cmp(void *a, void *b, void*) {
    return cmp_ivec2s(voidp_to_ivec2s(a), voidp_to_ivec2s(b));
}

int level_path_default_weight(const level *l, ivec2s p, void*) {
    if (!level_tile_in_bounds(p)) {
        return -1;
    } else if (l->tiles[p.x][p.y] != TILE_BASE) {
        return 10;
    }

    return 1;
}

// weight(...) returns < 0 if traversal is not possible
bool level_path(
    level *level,
    DYNLIST(ivec2s) *dst,
    ivec2s start,
    ivec2s goal,
    int (*weight)(const struct level_s*, ivec2s, void*),
    void *userptr) {
    struct map open_set, came_from, g_score, f_score;
    map_init(
        &open_set, level_path__hash, NULL, NULL, NULL,
        level_path__cmp, NULL, NULL, NULL);
    map_init(
        &came_from, level_path__hash, NULL, NULL, NULL,
        level_path__cmp, NULL, NULL, NULL);
    map_init(
        &g_score, level_path__hash, NULL, NULL, NULL,
        level_path__cmp, NULL, NULL, NULL);
    map_init(
        &f_score, level_path__hash, NULL, NULL, NULL,
        level_path__cmp, NULL, NULL, NULL);

    map_insert(&open_set, ivec2s_to_voidp(start), NULL);
    map_insert(&g_score, ivec2s_to_voidp(start), 0);
    map_insert(&f_score, ivec2s_to_voidp(start), 0);

    bool success = false;

#define LEVEL_PATH_MAX_ITERS 1024

    int n = 0;
    while (!map_empty(&open_set)) {
        if (n >= LEVEL_PATH_MAX_ITERS) {
            success = false;
            break;
        }

        n++;

        bool first = true;
        int current_fscore = INT32_MAX;
        ivec2s current;

        map_each(void*, void*, &open_set, it) {
            const ivec2s node_os =
                voidp_to_ivec2s(it.key);

            if (first) {
                current = node_os;
                first = false;
            }

            int *p = map_find(int, &g_score, ivec2s_to_voidp(node_os));
            const int node_fscore = p ? *p : INT32_MAX;

            if (node_fscore < current_fscore) {
                current = node_os;
                current_fscore = node_fscore;
            }
        }

        if (!cmp_ivec2s(current, goal)) {
            success = true;
            break;
        }

        map_remove(&open_set, ivec2s_to_voidp(current));

        for (direction d = DIRECTION_FIRST;
             d < DIRECTION_CARDINAL_COUNT;
             d++) {
            const ivec2s
                d_v = direction_to_ivec2s(d),
                n = {{ current.x + d_v.x, current.y + d_v.y }};

            const int w = weight(level, n, userptr);
            if (w < 0) {
                continue;
            }

            int *p = map_find(int, &g_score, ivec2s_to_voidp(n));
            const int
                gscore_tentative =
                    map_get(int, &g_score, ivec2s_to_voidp(current)) + w,
                gscore_n = p ? *p : INT32_MAX;

            if (gscore_tentative < gscore_n) {
                map_insert(
                    &came_from, ivec2s_to_voidp(n), ivec2s_to_voidp(current));
                map_insert(
                    &g_score, ivec2s_to_voidp(n), gscore_tentative);
                map_insert(
                    &f_score, ivec2s_to_voidp(n), gscore_tentative + 1);

                if (!map_contains(&open_set, ivec2s_to_voidp(n))) {
                    map_insert(&open_set, ivec2s_to_voidp(n), NULL);
                }
            }
        }
    }

    if (success) {
        // reconstruct path to dst
        ivec2s v = goal;
        *dynlist_insert(*dst, 0) = v;

        void **p = NULL;
        while ((p = map_find(void*, &came_from, ivec2s_to_voidp(v)))) {
            v = voidp_to_ivec2s(*p);
            *dynlist_insert(*dst, 0) = v;
        }
    }

    map_destroy(&open_set);
    map_destroy(&came_from);
    map_destroy(&f_score);
    map_destroy(&g_score);
    return success;
}

