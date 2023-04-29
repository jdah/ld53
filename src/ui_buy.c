#include "ui_buy.h"
#include "cjam/box.h"
#include "entity.h"
#include "font.h"
#include "gfx.h"
#include "palette.h"
#include "state.h"
#include "util.h"

#define GRID_WIDTH 3
#define GRID_HEIGHT 5
#define GRID_SIZE ((ivec2s) {{ GRID_WIDTH, GRID_HEIGHT }})

static const entity_type entities[GRID_WIDTH][GRID_HEIGHT] = {
    [0][GRID_HEIGHT - 1] = ENTITY_TURRET_L0,
    [1][GRID_HEIGHT - 1] = ENTITY_TURRET_L1,
    [2][GRID_HEIGHT - 1] = ENTITY_TURRET_L2,
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

static box get_box_grid(ivec2s index) {
    const ivec2s pos = get_pos_grid(index);
    return BOX_PS(pos, ((ivec2s) {{ 12, 12 }}));
}

static ivec2s get_pos_button() {
    const ivec2s base = get_pos_base();
    return (ivec2s) {{
        base.x + 0,
        base.y + 0,
    }};
}

static box get_box_button() {
    const ivec2s pos = get_pos_button();
    return BOX_PS(
        ((ivec2s) {{ pos.x + 1, pos.y + 1 }}),
        ((ivec2s) {{ 38, 10 }}));
}

void ui_buy_update() {
    for (int y = 0; y < GRID_SIZE.y; y++) {
        for (int x = 0; x < GRID_SIZE.x; x++) {
            const ivec2s index = {{ x, y }};
            const ivec2s pos = get_pos_grid((ivec2s) {{ x, y }});
            const bool highlight =
                box_contains(get_box_grid(index), state->input.cursor.pos);

            const entity_type entity = entities[index.x][index.y];
            entity_info *info = &ENTITY_INFO[entity];

            const bool unlocked = state->stats.unlocked[entity];
        }
    }
}

void ui_buy_render() {
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
                box_contains(get_box_grid(index), state->input.cursor.pos);

            const entity_type entity = entities[index.x][index.y];
            entity_info *info = &ENTITY_INFO[entity];

            const bool unlocked = state->stats.unlocked[entity];

            vec4s color = COLOR_WHITE;

            if (highlight) {
                color = GRAYSCALE(1.5f, 1.0f);
            } else if (!unlocked) {
                color = GRAYSCALE(0.6f, 1.0f);
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
            box_contains(get_box_button(), state->input.cursor.pos);
        const char *text;
        ivec2s offset, pos = get_pos_button();
        vec4s color;

        if (state->state == GAME_STATE_BUILD) {
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
