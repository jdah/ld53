#include "ui.h"
#include "state.h"
#include "ui_buy.h"

void ui_update() {
    if (state->state == GAME_STATE_BUILD) {
        ui_buy_update();
    }
}

void ui_render() {
    if (state->state == GAME_STATE_BUILD) {
        ui_buy_render();
    }
}
