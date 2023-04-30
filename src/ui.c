#include "ui.h"
#include "entity.h"
#include "font.h"
#include "gfx.h"
#include "level.h"
#include "level_data.h"
#include "palette.h"
#include "sound.h"
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
    [0][GRID_HEIGHT - 4] = ENTITY_DECOY_TRUCK,
    [1][GRID_HEIGHT - 4] = ENTITY_CANNON_L0,
    [2][GRID_HEIGHT - 4] = ENTITY_CANNON_L1,
    /* [1][GRID_HEIGHT - 3] = ENTITY_RADAR_L1, */
    /* [2][GRID_HEIGHT - 3] = ENTITY_BOOMBOX, */
    [0][GRID_HEIGHT - 5] = ENTITY_REPAIR,
    [1][GRID_HEIGHT - 5] = ENTITY_ARMOR_UPGRADE,
    [2][GRID_HEIGHT - 5] = ENTITY_SPEED_UPGRADE,
};

static ivec2s get_pos_base() {
    const sg_image_desc imdesc = sg_query_image_desc(state->image.buy_base);
    return (ivec2s) {{ TARGET_SIZE.x - imdesc.width, 0 }};
}

static ivec2s get_size_base() {
    const sg_image_desc imdesc = sg_query_image_desc(state->image.buy_base);
    return IVEC2S(imdesc.width, imdesc.height);
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

static int price_for_entity(entity_type type) {
    entity_info *info = &ENTITY_INFO[type];

    if (info->flags & EIF_NOT_AN_ENTITY) {
        switch (type) {
        default: break;
        case ENTITY_REPAIR: {
            return 50;
        } break;
        case ENTITY_ARMOR_UPGRADE: {
            return 450 + state->stats.truck_armor_level * 300;
        } break;
        case ENTITY_SPEED_UPGRADE: {
            return 500 + state->stats.truck_speed_level * 300;
        } break;
        }
    }

    return state->stats.unlocked[type] ? info->buy_price : info->unlock_price;
}

static bool can_buy_entity(entity_type type) {
    entity_info *info = &ENTITY_INFO[type];

    if (info->flags & EIF_NOT_AN_ENTITY) {
        switch (type) {
        default: ASSERT(false);
        case ENTITY_REPAIR: {
            return state->stats.truck_health < MAX_TRUCK_HEALTH;
        } break;
        case ENTITY_ARMOR_UPGRADE: {
            return state->stats.truck_armor_level != MAX_TRUCK_ARMOR;
        } break;
        case ENTITY_SPEED_UPGRADE: {
            return state->stats.truck_speed_level != MAX_TRUCK_SPEED;
        } break;
        }
    }

    return true;
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

            const entity_type type = buy_entities[index.x][index.y];
            entity_info *info = &ENTITY_INFO[type];

            if (info->name) {
                ui_set_desc("%s", info->name);
            }

            if (!select) {
                continue;
            }

            if (!can_buy_entity(type)) {
                continue;
            }

            // cancel all actions
            if (type == ENTITY_TYPE_NONE) {
                state->ui.place_entity = ENTITY_TYPE_NONE;
                return true;
            }

            const bool unlocked = state->stats.unlocked[type];
            const int price = price_for_entity(type);

            if (state->stats.money < price) {
                sound_play("select_lo.wav", 1.0f);
                continue;
            }

            if (!unlocked) {
                sound_play("unlock.wav", 1.0f);
                state->stats.unlocked[type] = true;
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

            if (info->flags & EIF_NOT_AN_ENTITY) {
                switch (type) {
                default: ASSERT(false); break;
                case ENTITY_REPAIR: {
                    entity *truck =
                        level_find_entity(state->level, ENTITY_TRUCK);

                    if (truck) {
                        truck->health =
                            clamp(truck->health + 10, 0, MAX_TRUCK_HEALTH);
                        state->stats.truck_health = truck->health;
                    } else {
                        state->stats.truck_health =
                            clamp(
                                state->stats.truck_health + 10, 0, MAX_TRUCK_HEALTH);
                    }
                } break;
                case ENTITY_ARMOR_UPGRADE: {
                    state->stats.truck_armor_level++;
                } break;
                case ENTITY_SPEED_UPGRADE: {
                    state->stats.truck_speed_level++;
                } break;
                }

                state->stats.money -= price;
                particle *p =
                    particle_new_text(
                        IVEC2S2V(state->input.cursor.pos),
                        palette_get(PALETTE_RED),
                        TICKS_PER_SECOND,
                        "-%d",
                        price);
                p->z = Z_UI - 0.004f;
            } else {
                state->ui.place_entity = type;
            }

            sound_play("select_hi.wav", 1.0f);
            LOG("placing %d", type);
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
                    sound_play("explode.wav", 1.0f);

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
                sound_play("select_hi.wav", 1.0f);
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
            bool highlight =
                aabb_contains(get_aabb_grid(index), state->input.cursor.pos);

            const entity_type type = buy_entities[index.x][index.y];
            entity_info *info = &ENTITY_INFO[type];

            bool unlocked = state->stats.unlocked[type];
            bool enabled = true, show_price = true;

            if (!can_buy_entity(type)) {
                enabled = false;
                show_price = false;
            }

            vec4s color = COLOR_WHITE;

            if (highlight) {
                color = GRAYSCALE(1.5f, 1.0f);
            } else if (!unlocked || !enabled) {
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


            if (type != ENTITY_TYPE_NONE) {
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

            if (highlight && show_price) {
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

                const int price = price_for_entity(type);
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
                    price);
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
        state->stats.truck_armor_level > 0 ?
            palette_get(11)
            : COLOR_WHITE,
        FONT_DOUBLED,
        "%d",
        (int) (truck ? truck->health : state->stats.truck_health));
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

static int get_building_reclaim_bonus(level *l) {
    int res = 0;

    dlist_each(node, &l->all_entities, it) {
        if (E_INFO(it.el)->flags & EIF_PLACEABLE) {
            const int max_health = E_INFO(it.el)->max_health;
            const f32 condition =
                max_health > 0 ?
                    ifnan(it.el->health / (f32) E_INFO(it.el)->max_health, 0)
                    : 1;
            res += E_INFO(it.el)->buy_price * condition;
        }
    }

    return res;
}

static void draw_endgame() {
    if (state->stage != STAGE_DONE) {
        return;
    }

    entity *truck = level_find_entity(state->level, ENTITY_TRUCK);
    const bool lost = !truck || truck->health <= 0;
    const ivec2s size_base = get_size_base();

    if (lost) {
        const char *done_text = "$60FAILURE TO\n DELIVER";
        int width = font_width(done_text);
        int x = ((TARGET_SIZE.x - size_base.x) - width) / 2;
        int y = (TARGET_SIZE.y / 2) + 16;
        font_str(
            IVEC2S(x, y),
            Z_UI,
            COLOR_WHITE,
            FONT_DOUBLED,
            done_text);
        y -= 22;

        const vec4s color = palette_get(6);
        font_v(
            (ivec2s) {{ x - 12, y }},
            Z_UI,
            VEC4S(color.r, color.g, color.b, 0.8f + 0.2f * sin(state->time.tick / 6.0f)),
            FONT_DOUBLED,
            " PRESS SPACE\nTO ADMIT DEFEAT\n\n"
            " $05   [R] TO \n  TRY AGAIN");
        y -= 10;
        return;
    }

    const int since = state->time.tick - state->stage_change_tick;
    if (state->done_screen_state == 0) {
        state->done_screen_state++;
        sound_play("explode.wav", 1.0f);
    }

    const char *done_text = " PACKAGE\n$35DELIVERED";
    int width = font_width(done_text);
    int x = ((TARGET_SIZE.x - size_base.x) - width) / 2;
    int y = (TARGET_SIZE.y / 2) + 16;
    font_str(
        IVEC2S(x, y),
        Z_UI,
        COLOR_WHITE,
        FONT_DOUBLED,
        done_text);
    y -= 22;

    if (since >= 30) {
        if (state->done_screen_state == 1) {
            state->stats.money +=
                state->level->data->bonus;
            state->done_screen_state++;
            sound_play("explode.wav", 1.0f);
        }

        font_v(
            (ivec2s) {{ x, y }},
            Z_UI,
            COLOR_WHITE,
            FONT_DOUBLED,
            "PAY: $33%d",
            state->level->data->bonus);
        y -= 10;
    }

    if (since >= 60) {
        const int truck_bonus = truck ? (truck->health / 2) : 0;
        if (state->done_screen_state == 2) {
            state->stats.money += truck_bonus;
            state->done_screen_state++;
            sound_play("explode.wav", 1.0f);
        }

        font_v(
            (ivec2s) {{ x, y }},
            Z_UI,
            COLOR_WHITE,
            FONT_DOUBLED,
            "   + $33%d",
            truck_bonus);

        gfx_batcher_push_sprite(
            &state->batcher,
            &state->atlas.tile,
            &(gfx_sprite) {
                .pos = (vec2s) {{ x + 14, y }},
                .color = COLOR_WHITE,
                .index = IVEC2S(4, 1),
                .z = Z_UI,
                .flags = GFX_NO_FLAGS
            });

        y -= 10;
    }

    // TODO:building bonus
    if (since >= 90) {
        const int build_bonus = get_building_reclaim_bonus(state->level);
        if (state->done_screen_state == 3) {
            state->stats.money += build_bonus;
            state->done_screen_state++;
            sound_play("explode.wav", 1.0f);
        }

        font_v(
            (ivec2s) {{ x, y }},
            Z_UI,
            COLOR_WHITE,
            FONT_DOUBLED,
            "   + $33%d",
            build_bonus);

        gfx_batcher_push_sprite(
            &state->batcher,
            &state->atlas.tile,
            &(gfx_sprite) {
                .pos = (vec2s) {{ x + 14, y }},
                .color = COLOR_WHITE,
                .index = IVEC2S(0, 5),
                .z = Z_UI,
                .flags = GFX_NO_FLAGS
            });

        y -= 10;
    }

    if (since >= 120) {
        if (state->done_screen_state == 4) {
            state->done_screen_state++;
            sound_play("win.wav", 1.0f);
        }

        const char *text;
        if (state->level_index == NUM_LEVELS - 1) {
            text = "PRESS SPACE\n  TO $32WIN ";
        } else {
            text = " PRESS SPACE\nFOR NEXT LEVEL";
        }

        const vec4s color = palette_get(PALETTE_ALIEN_GREEN);
        font_v(
            (ivec2s) {{ x - 12, y }},
            Z_UI,
            VEC4S(color.r, color.g, color.b, 0.8f + 0.2f * sin(state->time.tick / 6.0f)),
            FONT_DOUBLED,
            text);
        y -= 10;
    }
}

static void update_endgame() {
    if (state->stage != STAGE_DONE) {
        return;
    }

    entity *truck = level_find_entity(state->level, ENTITY_TRUCK);
    const bool lost = !truck || truck->health <= 0;

    if (lost && (input_get(&state->input, "r") & INPUT_PRESS)) {
        state->stats = state->old_stats;
        state_set_stage(state, STAGE_TITLE);
        state_set_level(state, state->level_index);
    }

    if (input_get(&state->input, "space") & INPUT_PRESS) {
        if (lost) {
            state_set_stage(state, STAGE_MAIN_MENU);
        } else if (state->level_index == NUM_LEVELS - 1) {
            sound_play("win.wav", 1.0f);
            state_set_stage(state, STAGE_MAIN_MENU);
            state->has_won = true;
        } else {
            state_set_stage(state, STAGE_TITLE);
            state_set_level(state, state->level_index + 1);
        }
    }
}

void ui_draw() {
    draw_sidebar();
    draw_stats();
    draw_overlay();
    draw_desc();
    draw_advice();
    draw_endgame();
}

void ui_update() {
    update_endgame();

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

int ui_center_str(
    int y,
    f32 z,
    vec4s color,
    const char *fmt,
    ...) {
    va_list ap;
    va_start(ap);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    const int width = font_width(buf);
    font_str(
        (ivec2s) {{ (TARGET_SIZE.x - width) / 2, y }},
        z,
        color,
        FONT_DOUBLED,
        buf);

    return (TARGET_SIZE.x - width) / 2;
}
