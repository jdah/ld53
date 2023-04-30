#pragma once

#include <SDL.h>

#include "defs.h"
#include "input.h"
#include "gfx.h"
#include "main_menu.h"
#include "particle.h"

#include <cjam/dynlist.h>

typedef struct level_s level;

typedef struct {
    int money;
    f32 truck_health;

    int truck_armor_level;
    int truck_speed_level;

    bool unlocked[ENTITY_TYPE_COUNT];
} stats;

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
        sg_image logo;
        sg_image win_overlay;
    } image;

    struct {
        u64 frame_start, last_second;
        u64 tick_remainder;
        u64 frame_ticks;
        u64 delta;
        u64 second_ticks, tps;
        u64 second_frames, fps;
        u64 tick;
        u64 animtick;
    } time;

    stats stats, old_stats;

    struct {
        char desc[256];
        entity_type place_entity;
    } ui;

    bool has_won;

    // TODO
    DYNLIST(particle) particles;

    // current game state
    stage stage, last_stage;

    int stage_change_tick;

    int done_screen_state;

    main_menu _main_menu;

    cursor_mode cursor_mode;

    int title_state;

    int level_index;
    level *level;

    bool hacks;

    vec4s clear_color;
} global_state;

extern global_state *state;

ALWAYS_INLINE void state_set_stage(global_state *s, stage stage) {
    LOG("from stage %d -> %d", s->stage, stage);
    s->stage = stage;
    s->stage_change_tick = s->time.tick;
    s->title_state = 0;
}

void state_set_level(global_state *s, int level);
