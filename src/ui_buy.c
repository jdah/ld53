#include "ui_buy.h"
#include "font.h"
#include "gfx.h"
#include "state.h"
#include "util.h"

void ui_buy_update() {

}

void ui_buy_render() {
    const sg_image_desc imdesc = sg_query_image_desc(state->image.buy_base);
    const ivec2s pos_base = (ivec2s) {{ TARGET_SIZE.x - imdesc.width, 0 }};
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

    const ivec2s pos_grid = {{
        pos_base.x + 1,
        pos_base.y + 14
    }};

    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 3; x++) {
            gfx_batcher_push_sprite(
                &state->batcher,
                &state->atlas.ui,
                &(gfx_sprite) {
                    .pos = {{
                        pos_grid.x + (x * 13),
                        pos_grid.y + (y * 13),
                    }},
                    .index = {{ 0, 0 }},
                    .color = COLOR_WHITE,
                    .z = Z_UI - 0.001f,
                    .flags = GFX_NO_FLAGS
                });
        }
    }
}
