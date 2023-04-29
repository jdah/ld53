#include "level.h"
#include "cjam/time.h"
#include "defs.h"
#include "state.h"

#include <cjam/rand.h>

const char char_to_tile[256] = {
    ['?'] = TILE_NONE,
    [' '] = TILE_BASE,
    ['r'] = TILE_ROAD,
};

const char *map[LEVEL_HEIGHT] = {
    "              r         ",
    "              r         ",
    "              r         ",
    "              r         ",
    "        rrrrrrr         ",
    "        r               ",
    "        r               ",
    "rrrrrrrrr               ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
};

void level_init(level *level) {
    memset(level->tiles, 0, sizeof(level->tiles));
    level->tiles[1][1] = TILE_BASE;
    level->tiles[LEVEL_WIDTH - 1][LEVEL_HEIGHT - 1] = TILE_BASE;

    for (int x = 0; x < LEVEL_WIDTH; x++) {
        for (int y = 0; y < LEVEL_HEIGHT; y++) {
            level->tiles[x][y] = char_to_tile[(int) map[LEVEL_HEIGHT - y - 1][x]];
        }
    }
}

static void tile_draw(
    const level *level, ivec2s lpos, tile_type tile) {
    ivec2s index = {{ 0, 0 }};
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
    } break;
    }

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ lpos.x * TILE_SIZE_PX, lpos.y * TILE_SIZE_PX }},
            .index = index,
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_LEVEL_BASE,
            .flags = GFX_NO_FLAGS
        });
}

static void object_draw(
    const level *level, ivec2s lpos, object_type object) {
    ivec2s index = {{ 0, 0 }};

    switch (object) {
    case OBJECT_COUNT: ASSERT(false);
    case OBJECT_NONE: return;
    case OBJECT_TURRET_L0: {
        index = (ivec2s) {{ 0, 5 }};

        const int i = ((int) (time_ns() / 1000000.0f)) % 6;
        gfx_batcher_push_sprite(
            &state->batcher,
            &state->atlas.tile,
            &(gfx_sprite) {
                .pos = {{ lpos.x * TILE_SIZE_PX, lpos.y * TILE_SIZE_PX }},
                .index = {{
                    0 + i, 6
                }},
                .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
                .z = Z_LEVEL_OBJECT_OVERLAY,
                .flags = GFX_NO_FLAGS
            });
    } break;
    }

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .pos = {{ lpos.x * TILE_SIZE_PX, lpos.y * TILE_SIZE_PX }},
            .index = index,
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_LEVEL_OBJECT,
            .flags = GFX_NO_FLAGS
        });
}

void level_draw(const level *level) {
    for (int x = 0; x < LEVEL_WIDTH; x++) {
        for (int y = 0; y < LEVEL_HEIGHT; y++) {
            tile_draw(level, (ivec2s) {{ x, y }}, level->tiles[x][y]);
            object_draw(level, (ivec2s) {{ x, y }}, level->objects[x][y]);
        }
    }
}
