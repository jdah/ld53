#include "main_menu.h"
#include "font.h"
#include "gfx.h"
#include "sound.h"
#include "state.h"
#include "ui.h"
#include "palette.h"
#include "util.h"

static const char *mmi_names[MMI_COUNT] = {
    "PLAY",
    "STORY",
    "HOW TO"
};

static const char *mmp_texts[MMP_COUNT] = {
    [MMP_MAIN] = "ERROR",
    [MMP_STORY] =
        "$09YOU RUN A $32PRECIOUS METAL$09\n"
        "SHIPPING COMPANY ON THE \n"
        "PLANET $56NEPTUNE$09. THE     \n"
        "PLANET'S INHABITANTS,   \n"
        "$36ALIENS$09, ARE NOT PLEASED \n"
        "ABOUT THIS.             \n"
        "                        \n"
        "                        \n"
        "$61DEFEND YOUR CARGO.      \n"
        "$62MAKE YOUR DELIVERY.     \n",
    [MMP_HOW_TO] =
        "$09USE ALL AVAILABLE TOOLS \n"
        "IN YOUR ARSENAL TO GET  \n"
        "YOUR TRUCK TO THE NEXT  \n"
        "WAREHOUSE.              \n"
        "                        \n"
        "BUILD, AND WHEN READY,  \n"
        "PRESS \"GO!\".          \n"
        "                        \n"
        "$05HINT: $36ALIENS$05 ARE NOT    \n"
        "FOND OF SMOOTH JAZZ     \n"
};

static void text_page_draw(main_menu *mm, int page) {
    font_str(
        IVEC2S(2, TARGET_SIZE.y - 10),
        Z_UI - 0.001f,
        COLOR_WHITE,
        FONT_DOUBLED,
        mmp_texts[page]);

    ui_center_str(
        2,
        Z_UI - 0.001f,
        COLOR_WHITE,
        "[$33SPACE$08] TO RETURN");
}

void main_menu_draw(main_menu *mm) {
    if (mm->page == MMP_MAIN) {
        state->clear_color = palette_get(PALETTE_SKY);

        const int offset = sinf(state->time.tick / 10) < 0 ? -1 : 0;
        const ivec2s logo_pos =
            (ivec2s) {{ (TARGET_SIZE.x - 128) / 2, offset + (TARGET_SIZE.y - 48) }};

        gfx_batcher_push_image(
            &state->batcher,
            state->image.logo,
            IVEC2S2V(logo_pos),
            COLOR_WHITE,
            Z_UI - 0.004f,
            GFX_NO_FLAGS);

        if (state->has_won) {
            gfx_batcher_push_image(
                &state->batcher,
                state->image.win_overlay,
                IVEC2S2V(logo_pos),
                COLOR_WHITE,
                Z_UI - 0.004f,
                GFX_NO_FLAGS);
        }

        int y = (TARGET_SIZE.y / 2) - 4;
        for (int i = 0; i < MMI_COUNT; i++) {
            const bool selected = mm->index == i;

            char buf[256];
            snprintf(
                buf, sizeof(buf), "%s%s%s",
                selected ? ">" : "",
                mmi_names[i],
                selected ? "<" : "");
            ui_center_str(
                y,
                Z_UI - 0.004f,
                selected ?
                    palette_get(PALETTE_WHITE)
                    : palette_get(PALETTE_LIGHT_GRAY),
                "%s", buf);
            y -= 10;
        }

        font_str(
            IVEC2S(2, 2),
            Z_UI - 0.005f,
            palette_get(PALETTE_SKY + 1),
            FONT_DOUBLED,
            "BY JDH");

        font_str(
            IVEC2S(TARGET_SIZE.x - 34, 2),
            Z_UI - 0.005f,
            palette_get(6),
            FONT_DOUBLED,
            "LD53");
    } else {
        text_page_draw(mm, mm->page);
    }
}

static void text_page_update(main_menu *mm, int page) {
    if (input_get(&state->input, "space|return") & INPUT_PRESS) {
        mm->page = MMP_MAIN;
    }
}

void main_menu_update(main_menu *mm) {
    if (mm->page == MMP_MAIN) {
        if (input_get(&state->input, "up") & INPUT_PRESS) {
            mm->index = clamp(mm->index - 1, 0, MMI_COUNT - 1);
            sound_play("boop.wav", 1.0f);
        }

        if (input_get(&state->input, "down") & INPUT_PRESS) {
            mm->index = clamp(mm->index + 1, 0, MMI_COUNT - 1);
            sound_play("boop.wav", 1.0f);
        }

        if (input_get(&state->input, "space|return") & INPUT_PRESS) {
            if (mm->index == MMI_START) {
                /* sound_play("title.wav", 1.0f); */
                state_set_stage(state, STAGE_TITLE);
                state_set_level(state, 9);
            } else {
                sound_play("select_hi.wav", 1.0f);
                mm->page = mm->index == MMI_STORY ? MMP_STORY : MMP_HOW_TO;
            }
        }
    } else {
        text_page_update(mm, mm->page);
    }
}
