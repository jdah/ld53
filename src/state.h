#pragma once

#include <SDL.h>

#include "defs.h"
#include "input.h"
#include "gfx.h"
#include "particle.h"

#include <cjam/dynlist.h>

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

    struct {
        int money;
        f32 health;

        bool unlocked[ENTITY_TYPE_COUNT];
    } stats;

    struct {
        char desc[256];
        entity_type place_entity;
    } ui;

    // TODO
    DYNLIST(particle) particles;

    // current game state
    stage stage, last_stage;

    level *level;
} global_state;

extern global_state *state;

ALWAYS_INLINE void state_set_stage(global_state *s, stage stage) {
    LOG("from stage %d -> %d", s->stage, stage);
    s->stage = stage;
}

