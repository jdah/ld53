#include "ui.h"
#include "entity.h"
#include "font.h"
#include "gfx.h"
#include "level.h"
#include "palette.h"
#include "state.h"
#include "util.h"

#include <stdarg.h>

#include <cjam/aabb.h>

#define GRID_WIDTH 3
#define GRID_HEIGHT 5
#define GRID_SIZE ((ivec2s) {{ GRID_WIDTH, GRID_HEIGHT }})

static const entity_type buy_entities[GRID_WIDTH][GRID_HEIGHT] = {
    [0][GRID_HEIGHT - 1] = ENTITY_TURRET_L0,
    [1][GRID_HEIGHT - 1] = ENTITY_TURRET_L1,
    [2][GRID_HEIGHT - 1] = ENTITY_TURRET_L2,
    [0][GRID_HEIGHT - 2] = ENTITY_MINE_L0,
    [1][GRID_HEIGHT - 2] = ENTITY_MINE_L1,
    [2][GRID_HEIGHT - 2] = ENTITY_MINE_L2,
    [0][GRID_HEIGHT - 3] = ENTITY_RADAR_L0,
    [1][GRID_HEIGHT - 3] = ENTITY_RADAR_L1,
    [2][GRID_HEIGHT - 3] = ENTITY_BOOMBOX,
};

static ivec2s get_pos_base() {
    const sg_image_desc imdesc = sg_query_image_desc(state->image.buy_base);
    return (ivec2s) {{ TARGET_SIZE.x - imdesc.width, 0 }};
}

static ivec2s get_pos_grid(ivec2s index) {
    const ivec2s base = get_pos_base();
    const ivec2s pos_grid = {{
        base.x + 1,
        base.y + 28
    }};
    return (ivec2s) {{
        pos_grid.x + (index.x * 13),
        pos_grid.y + (index.y * 13),
    }};
}

static aabb get_aabb_grid(ivec2s index) {
    const ivec2s pos = get_pos_grid(index);
    return AABB_PS(pos, ((ivec2s) {{ 12, 12 }}));
}

static ivec2s get_pos_button() {
    const ivec2s base = get_pos_base();
    return (ivec2s) {{
        base.x + 0,
        base.y + 0,
    }};
}

static aabb get_aabb_button() {
    const ivec2s pos = get_pos_button();
    return AABB_PS(
        ((ivec2s) {{ pos.x + 1, pos.y + 1 }}),
        ((ivec2s) {{ 38, 10 }}));
}

static bool can_buy() {
    return state->stage == STAGE_BUILD || state->stage == STAGE_PLAY;
}

bool update_sidebar() {
    if (!can_buy()) {
        return false;
    }

    const bool select = input_get(&state->input, "mouse_left") & INPUT_PRESS;

    for (int y = 0; y < GRID_SIZE.y; y++) {
        for (int x = 0; x < GRID_SIZE.x; x++) {
            const ivec2s index = {{ x, y }};
            if (!aabb_contains(get_aabb_grid(index), state->input.cursor.pos)) {
                continue;
            }

            const entity_type entity = buy_entities[index.x][index.y];
            entity_info *info = &ENTITY_INFO[entity];

            if (info->name) {
                ui_set_desc("%s", info->name);
            }

            if (!select) {
                continue;
            }

            // cancel all actions
            if (entity == ENTITY_TYPE_NONE) {
                state->ui.place_entity = ENTITY_TYPE_NONE;
                return true;
            }

            const bool unlocked = state->stats.unlocked[entity];
            const int price =
                unlocked ? info->buy_price : info->unlock_price;

            if (state->stats.money < price) {
                // TODO: play bad sound
                continue;
            }

            if (!unlocked) {
                // TODO: sound
                state->stats.unlocked[entity] = true;
                state->stats.money -= price;

                particle *p =
                    particle_new_text(
                        IVEC2S2V(state->input.cursor.pos),
                        palette_get(PALETTE_RED),
                        TICKS_PER_SECOND,
                        "-%d",
                        price);
                p->z = Z_UI - 0.004f;
            }

            state->ui.place_entity = entity;
            LOG("placing %d", entity);
            return true;
        }
    }

    // button
    {
        const bool highlight =
            aabb_contains(get_aabb_button(), state->input.cursor.pos);

        if (highlight
            && state->stage == STAGE_BUILD
            && select) {
            state_set_stage(state, STAGE_PLAY);
        }
    }

    return false;
}

static bool update_cursor() {
    if (!can_buy()) {
        state->ui.place_entity = ENTITY_TYPE_NONE;
        return false;
    }

    if (input_get(&state->input, "x") & INPUT_PRESS) {
        state->cursor_mode =
            state->cursor_mode == CURSOR_MODE_DELETE ?
                CURSOR_MODE_DEFAULT
                : CURSOR_MODE_DELETE;
    }

    const bool
        select = input_get(&state->input, "mouse_left") & INPUT_PRESS,
        deselect = input_get(&state->input, "mouse_right") & INPUT_PRESS;

    if (state->cursor_mode == CURSOR_MODE_DELETE) {
        state->ui.place_entity = ENTITY_TYPE_NONE;

        if (deselect) {
            state->cursor_mode = CURSOR_MODE_DEFAULT;
            return true;
        }

        if (select) {
            const ivec2s tile = state->input.cursor.tile;
            dlist_each(tile_node, &state->level->tile_entities[tile.x][tile.y], it) {
                entity_info *info = &ENTITY_INFO[it.el->type];
                if (info->flags & EIF_PLACEABLE) {
                    // reclaim
                    it.el->delete = true;

                    const int amount = info->buy_price / 2;
                    state->stats.money += amount;
                    particle_new_text(
                        IVEC2S2V(state->input.cursor.pos),
                        palette_get(PALETTE_YELLOW),
                        TICKS_PER_SECOND,
                        "+%d",
                        amount);
                }
            }

            return true;
        }
    }

    if (state->ui.place_entity != ENTITY_TYPE_NONE) {
        entity_info *info = &ENTITY_INFO[state->ui.place_entity];

        if (state->stats.money < info->buy_price) {
            state->ui.place_entity = ENTITY_TYPE_NONE;
        } else if (select) {
            const bool
                in_level = state->input.cursor.in_level,
                can_place =
                    in_level &&
                    (!info->can_place || info->can_place(state->input.cursor.tile));

            if (in_level && can_place) {
                // TODO: happy sound
                state->stats.money -= info->buy_price;

                particle_new_text(
                    IVEC2S2V(state->input.cursor.tile_px),
                    palette_get(PALETTE_RED),
                    TICKS_PER_SECOND,
                    "-%d",
                    info->buy_price);

                entity *e = level_new_entity(state->level, state->ui.place_entity);
                entity_set_pos(e, IVEC2S2V(state->input.cursor.tile_px));
            } else if (!in_level) {
                LOG("cancelling placing %d, out of level", state->ui.place_entity);
                state->ui.place_entity = ENTITY_TYPE_NONE;
            }

            return true;
        } else if (deselect) {
            state->ui.place_entity = ENTITY_TYPE_NONE;
            return true;
        }
    }

    return false;
}

static void draw_sidebar() {
    const sg_image_desc imdesc = sg_query_image_desc(state->image.buy_base);
    const ivec2s pos_base = get_pos_base();
    gfx_batcher_push_image(
        &state->batcher,
        state->image.buy_base,
        IVEC2S2V(pos_base),
        COLOR_WHITE,
        Z_UI,
        GFX_NO_FLAGS);

    const char *title = "BUY";
    const int title_width = font_width(title);
    font_str(
        (ivec2s) {{
            (TARGET_SIZE.x - imdesc.width)
                + ((imdesc.width - title_width) / 2),
            TARGET_SIZE.y - 10,
        }},
        Z_UI - 0.001f,
        COLOR_WHITE,
        FONT_DOUBLED,
        title);

    for (int y = 0; y < GRID_SIZE.y; y++) {
        for (int x = 0; x < GRID_SIZE.x; x++) {
            const ivec2s index = {{ x, y }};
            const ivec2s pos = get_pos_grid((ivec2s) {{ x, y }});
            const bool highlight =
                aabb_contains(get_aabb_grid(index), state->input.cursor.pos);

            const entity_type entity = buy_entities[index.x][index.y];
            entity_info *info = &ENTITY_INFO[entity];

            const bool unlocked = state->stats.unlocked[entity];

            vec4s color = COLOR_WHITE;

            if (highlight) {
                color = GRAYSCALE(1.5f, 1.0f);
            } else if (!unlocked) {
                color = GRAYSCALE(0.4f, 1.0f);
            }

            gfx_batcher_push_sprite(
                &state->batcher,
                &state->atlas.ui,
                &(gfx_sprite) {
                    .pos = IVEC2S2V(pos),
                    .index = {{ 0, 0 }},
                    .color = color,
                    .z = Z_UI - 0.001f,
                    .flags = GFX_NO_FLAGS
                });


            if (entity != ENTITY_TYPE_NONE) {
                gfx_batcher_push_sprite(
                    &state->batcher,
                    &state->atlas.tile,
                    &(gfx_sprite) {
                        .pos = (vec2s) {{
                            pos.x + 2,
                            pos.y + 3
                        }},
                        .index = info->base_sprite,
                        .color = color,
                        .z = Z_UI - 0.002f,
                        .flags = GFX_NO_FLAGS
                    });
            }

            if (highlight) {
                const ivec2s icon = unlocked ? IVEC2S(0, 0) : IVEC2S(2, 0);

                gfx_batcher_push_sprite(
                    &state->batcher,
                    &state->atlas.icon,
                    &(gfx_sprite) {
                        .pos = (vec2s) {{
                            pos_base.x + 2,
                            pos_base.y + 13
                        }},
                        .index = icon,
                        .color = color,
                        .z = Z_UI - 0.002f,
                        .flags = GFX_NO_FLAGS
                    });

                const int price =
                    unlocked ? info->buy_price : info->unlock_price;
                font_v(
                    (ivec2s) {{
                        pos_base.x + 9,
                        pos_base.y + 13,
                    }},
                    Z_UI - 0.002f,
                    price <= state->stats.money ?
                        COLOR_WHITE
                        : palette_get(60),
                    FONT_DOUBLED,
                    "%d",
                    unlocked ? info->buy_price : info->unlock_price);
            }
        }
    }

    // button
    {
        const bool highlight =
            aabb_contains(get_aabb_button(), state->input.cursor.pos);
        const char *text;
        ivec2s offset, pos = get_pos_button();
        vec4s color;

        if (state->stage == STAGE_BUILD) {
            text = "GO!";
            offset = (ivec2s) {{ 0, 16 }};
            color = GRAYSCALE(highlight ? 1.5f : 1.0f, 1.0f);
        } else {
            text = "GO!";
            offset = (ivec2s) {{ 0, 32 }};
            color = COLOR_WHITE;
        }

        gfx_batcher_push_subimage(
            &state->batcher,
            state->atlas.ui.image,
            IVEC2S2V(pos),
            color,
            Z_UI - 0.002f,
            GFX_NO_FLAGS,
            offset,
            (ivec2s) {{ 40, 16 }});

        const int width = font_width(text);
        font_str(
            (ivec2s) {{
                pos.x + (40 - width) / 2,
                pos.y + 2
            }},
            Z_UI - 0.003f,
            color,
            FONT_DOUBLED,
            text);
    }
}

static void draw_stats() {
    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.icon,
        &(gfx_sprite) {
            .index = {{ 0, 0 }},
            .pos = {{ 2, TARGET_SIZE.y - 9 }},
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_UI,
            .flags = GFX_NO_FLAGS
        });

    font_v(
        (ivec2s) {{ 2 + 9, TARGET_SIZE.y - 10 }},
        Z_UI,
        COLOR_WHITE,
        FONT_DOUBLED,
        "%d",
        state->stats.money);

    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .index = {{ 4, 1 }},
            .pos = {{ 2, TARGET_SIZE.y - 19 }},
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_UI,
            .flags = GFX_NO_FLAGS
        });


    entity *truck = level_find_entity(state->level, ENTITY_TRUCK);
    font_v(
        (ivec2s) {{ 2 + 9, TARGET_SIZE.y - 19 }},
        Z_UI,
        COLOR_WHITE,
        FONT_DOUBLED,
        "%d",
        truck ? (int) truck->health : 100);
}

static void draw_overlay() {
    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .index = {{ 0, 15 }},
            .pos = IVEC2S2V(state->input.cursor.tile_px),
            .color =
                state->cursor_mode == CURSOR_MODE_DEFAULT ?
                    COLOR_WHITE
                    : palette_get(PALETTE_RED),
            .z = Z_UI_LEVEL_OVERLAY,
            .flags = GFX_NO_FLAGS
        });

    if (state->ui.place_entity != ENTITY_TYPE_NONE) {
        gfx_batcher_push_sprite(
            &state->batcher,
            &state->atlas.tile,
            &(gfx_sprite) {
                .index = ENTITY_INFO[state->ui.place_entity].base_sprite,
                .pos = IVEC2S2V(state->input.cursor.tile_px),
                .color = GRAYSCALE(1.0f, 0.7f + sinf(state->time.tick / 5.0f) * 0.2f),
                .z = Z_UI_LEVEL_OVERLAY - 0.001f,
                .flags = GFX_NO_FLAGS
            });
    }
}

static void draw_desc() {
    if (!strlen(state->ui.desc)) { return; }

    const sg_image_desc imdesc = sg_query_image_desc(state->image.buy_base);
    const int width = font_width(state->ui.desc);
    font_str(
        (ivec2s) {{
            ((TARGET_SIZE.x - imdesc.width) - width) / 2,
            2,
        }},
        Z_UI,
        COLOR_WHITE,
        FONT_DOUBLED,
        state->ui.desc);
}

void draw_advice() {
    if (state->stage == STAGE_BUILD) {
        const char *text = "[$61X$08]RECLAIM";
        const int width = font_width(text);
        const sg_image_desc imdesc = sg_query_image_desc(state->image.buy_base);
        font_str(
            (ivec2s) {{
                ((TARGET_SIZE.x - imdesc.width) - width) / 2 + 24,
                TARGET_SIZE.y - 10,
            }},
            Z_UI,
            COLOR_WHITE,
            FONT_DOUBLED,
            text);
    }
}

void ui_draw() {
    draw_sidebar();
    draw_stats();
    draw_overlay();
    draw_desc();
    draw_advice();
}

void ui_update() {
    state->ui.desc[0] = '\0';

    bool got_cursor = false;
    if (!got_cursor) { got_cursor |= update_sidebar(); }
    if (!got_cursor) { got_cursor |= update_cursor(); }
}

void ui_set_desc(const char *fmt, ...) {
    va_list ap;
    va_start(ap);
    vsnprintf(state->ui.desc, sizeof(state->ui.desc), fmt, ap);
    va_end(ap);
}
