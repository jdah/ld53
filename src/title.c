#include "title.h"
#include "font.h"
#include "level.h"
#include "sound.h"
#include "state.h"
#include "ui.h"

void title_draw() {
    state->clear_color = COLOR_BLACK;

    char buf0[256], buf1[256];
    buf1[0] = '\0';
    snprintf(buf0, sizeof(buf0), "LEVEL %d\n", state->level_index + 1);

    const int since = state->time.tick - state->stage_change_tick;
    if (state->title_state == 0) {
        sound_play("win.wav", 1.0f);
        state->title_state++;
    } else if (since == 40 && state->title_state == 1) {
        sound_play("explode.wav", 1.0f);
        state->title_state++;
    }
    if (since > 40) {
        snprintf(buf1, sizeof(buf1), "%s", state->level->data->title);
    }

    const int width = font_width(buf0);
    font_str(
        IVEC2S((TARGET_SIZE.x - width) / 2, (TARGET_SIZE.y / 2) + 10),
        Z_UI,
        COLOR_WHITE,
        FONT_DOUBLED,
        buf0);

    ui_center_str(
        (TARGET_SIZE.y / 2),
        Z_UI,
        COLOR_WHITE,
        "%s",
        buf1);

    if (since >= TITLE_TICKS) {
        state_set_stage(state, STAGE_BUILD);
    }
}
