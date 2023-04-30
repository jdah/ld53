#include "state.h"
#include "level.h"
#include "level_data.h"

void state_set_level(global_state *s, int level) {
    if (s->level) { level_destroy(s->level); }

    state->level_index = level;
    state->level = calloc(1, sizeof(*state->level));
    level_init(state->level, &LEVELS[level]);
}
