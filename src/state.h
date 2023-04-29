#pragma once

#include <SDL.h>

#include "input.h"
#include "gfx.h"

typedef struct level_s level;

typedef struct {
    bool quit;
    SDL_Window *window;
    SDL_GLContext *glctx;
    input input;

    gfx_batcher batcher;

    struct {
        gfx_atlas font;
        gfx_atlas tile;
    } atlas;

    level *level;
} global_state;

extern global_state *state;
