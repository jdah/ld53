#pragma once

#include <SDL.h>

#include "defs.h"
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
        gfx_atlas ui;
        gfx_atlas icon;
    } atlas;

    struct {
        sg_image buy_base;
    } image;

    struct {
        u64 frame_start, last_second;
        u64 tick_remainder;
        u64 frame_ticks;
        u64 delta;
        u64 second_ticks, tps;
        u64 second_frames, fps;
        u64 tick;
    } time;

    // current game state
    game_state state;

    level *level;
} global_state;

extern global_state *state;
