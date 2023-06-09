#include "palette.h"
#include "title.h"
#include "util.h"
#define CJAM_IMPL

#include <cjam/dlist.h>
#include <cjam/dynlist.h>
#include <cjam/file.h>
#include <cjam/rand.h>
#include <cjam/time.h>
#include <cjam/log.h>

#include "level_data.h"
#include "sound.h"
#include "ui.h"
#include "defs.h"
#include "entity.h"
#include "font.h"
#include "level.h"
#ifdef EMSCRIPTEN
    #include <SDL.h>
    #include <SDL_image.h>
#else
    #include <SDL.h>
    #include <SDL_image.h>
#endif // ifdef EMSCRIPTEN

#ifdef EMSCRIPTEN
    #include <GLES3/gl3.h>
    #include "emscripten.h"
#else
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl3.h>
#endif // ifdef EMSCRIPTEN

#include <cglm/common.h>

#define SOKOL_IMPL
#define SOKOL_DEBUG

#ifdef EMSCRIPTEN
#   define SOKOL_GLES3
#else
#   define SOKOL_GLCORE33
#endif // ifdef EMSCRIPTEN

#include <sokol_gfx.h>

#ifdef EMSCRIPTEN
#   define GLSL_PREFIX "#version 300 es\n"
#else
#   define GLSL_PREFIX "#version 330\n"
#endif // ifdef EMSCRIPTEN

#include "gfx.h"
#include "input.h"
#include "state.h"

struct {
    sg_image color, depth;
    sg_pass pass;
    sg_pass_action passaction;
} offscreen;

static void _sg_logger(
        const char* tag,                // always "sg"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SG_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_gfx.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data) {
    // TODO: pretty filename
    log_write(
        filename_or_null ? filename_or_null : "(sokol_gfx.h)",
        line_nr,
        "",
        log_level <= 2 ? stderr : stdout,
        ((const char *[]) {
            "PAN",
            "ERR",
            "WRN",
            "LOG"
         })[log_level],
        "%s",
        message_or_null ? message_or_null : "(NULL)");
}


static void init() {
#ifdef HACKS
    state->hacks = true;
#endif

    state->time.last_second = time_ns();
/* #ifdef EMSCRIPTEN */
/*     const int res = EM_ASM_INT( */
/*         try { */
/*             return new AudioContext().state == 'running' ? 0 : 1; */
/*         } catch { */
/*             return 1; */
/*         } */
/*         return 0; */
/*     ); */
/*     /1* EM_JS(void, check_audio_ctx, (), { *1/ */
/*     /1*     /2* try { *2/ *1/ */
/*     /1*     /2*     new AudioContext(); *2/ *1/ */
/*     /1*     /2* } catch { *2/ *1/ */
/*     /1*     /2*     return false; *2/ *1/ */
/*     /1*     /2* } *2/ *1/ */
/*     /1*     /2* return true; *2/ *1/ */
/*     /1* }); *1/ */

/*     LOG("%d", res); */
/* #endif // ifdef EMSCRIPTEN */

    /* gfx_sound_init(); */

    input_init(&state->input);

    sg_setup(&(sg_desc) {
        .logger = (sg_logger) { .func = _sg_logger }
    });
    assert(sg_isvalid());

    offscreen.color =
        sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = TARGET_SIZE.x,
                .height = TARGET_SIZE.y,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .label = "offscreen.color"
            });

    offscreen.depth =
        sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = TARGET_SIZE.x,
                .height = TARGET_SIZE.y,
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .label = "offscreen.depth"
            });

    offscreen.pass =
        sg_make_pass(
            &(sg_pass_desc) {
                .color_attachments[0].image = offscreen.color,
                .depth_stencil_attachment.image = offscreen.depth,
                .label = "offscreen-pass"
            });

    gfx_batcher_init(&state->batcher);

    sg_image font_image;
    ASSERT(!gfx_load_image("font.png", &font_image));
    gfx_atlas_init(&state->atlas.font, font_image, (ivec2s) {{ 8, 8 }});

    sg_image tile_image;
    ASSERT(!gfx_load_image("tile.png", &tile_image));
    gfx_atlas_init(&state->atlas.tile, tile_image, (ivec2s) {{ 8, 8 }});

    sg_image ui_image;
    ASSERT(!gfx_load_image("ui.png", &ui_image));
    gfx_atlas_init(&state->atlas.ui, ui_image, (ivec2s) {{ 16, 16 }});

    sg_image icon_image;
    ASSERT(!gfx_load_image("icon.png", &icon_image));
    gfx_atlas_init(&state->atlas.icon, icon_image, (ivec2s) {{ 8, 8 }});

    ASSERT(!gfx_load_image("buy_base.png", &state->image.buy_base));
    ASSERT(!gfx_load_image("logo.png", &state->image.logo));
    ASSERT(!gfx_load_image("win_overlay.png", &state->image.win_overlay));

    state_set_stage(state, STAGE_MAIN_MENU);
}

static void deinit() {
    gfx_atlas_destroy(&state->atlas.font);
    gfx_atlas_destroy(&state->atlas.tile);
    gfx_atlas_destroy(&state->atlas.ui);
    gfx_atlas_destroy(&state->atlas.icon);
    gfx_batcher_destroy(&state->batcher);
}

static void frame() {
    if (state->hacks) {
        const bool
            skip = input_get(&state->input, "1") & INPUT_PRESS,
            cheat = input_get(&state->input, "2") & INPUT_PRESS;

        if (skip) {
            if (state->stage == STAGE_MAIN_MENU) {
                state_set_stage(state, STAGE_TITLE);
                state_set_level(state, 0);
            } else if (state->stage == STAGE_TITLE) {
                state_set_stage(state, STAGE_BUILD);
            } else if (state->stage == STAGE_PLAY) {
                // warp truck
                entity *truck = level_find_entity(state->level, ENTITY_TRUCK);
                if (truck) {
                    truck->truck.path_index = dynlist_size(truck->path) - 1;
                    entity_set_pos(
                        truck,
                        IVEC2S2V(level_tile_to_px(state->level->finish)));
                }

            } else if (state->stage == STAGE_BUILD) {
                state_set_stage(state, STAGE_PLAY);
            }
        }

        if (cheat) {
            if (state->stage == STAGE_PLAY || state->stage == STAGE_BUILD) {
                state->stats.money = 50000;
            }
        }
    }

    const u64 now = time_ns();
    state->time.delta =
        state->time.frame_start == 0 ? 16000 : (now - state->time.frame_start);
    state->time.frame_start = now;

    if (now - state->time.last_second > 1000000000) {
        state->time.tps = state->time.second_ticks;
        state->time.second_ticks = 0;
        state->time.fps = state->time.second_frames;
        state->time.second_frames = 0;
        state->time.last_second = now;
    }

    const u64 tick_time = state->time.delta + state->time.tick_remainder;
    state->time.frame_ticks = min(tick_time / NS_PER_TICK, TICKS_PER_SECOND);
    state->time.tick_remainder = tick_time % NS_PER_TICK;

    state->time.second_ticks += state->time.frame_ticks;
    state->time.second_frames++;

    input_update(&state->input);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            state->quit = true;
            break;
        }

        input_process(&state->input, &event);
    }

    // handle state transition
    if (state->last_stage != state->stage) {
        if (state->last_stage == STAGE_MAIN_MENU
            && state->stage == STAGE_TITLE) {
            LOG("STATS RESET");
            memset(&state->stats, 0, sizeof(state->stats));
            state->stats.money = START_MONEY;
            state->stats.truck_health = MAX_TRUCK_HEALTH;

            for (int i = 0; i < ENTITY_TYPE_COUNT; i++) {
                if (ENTITY_INFO[i].unlock_price == 0) {
                    state->stats.unlocked[i] = true;
                }
            }
        }

        if (state->last_stage == STAGE_TITLE) {
            // backup for restart
            state->old_stats = state->stats;
        }

        state->cursor_mode = CURSOR_MODE_DEFAULT;
        state->done_screen_state = 0;

        // TODO: clear particles?
        /* dynlist_resize(state->particles, 0); */
    }

    if (state->last_stage == STAGE_BUILD
        && state->stage == STAGE_PLAY) {
        level_go(state->level);
    }

    state->last_stage = state->stage;

    bool unpaused = false;
    if (state->stage == STAGE_MAIN_MENU) {
        main_menu_update(&state->_main_menu);
    } else if (state->paused) {
        font_str(
            (ivec2s) {{ 24, TARGET_SIZE.y / 2 }},
            Z_UI,
            VEC4S(1.0f),
            FONT_DOUBLED,
            "$61[SPACE] QUIT\n\n"
            "$07[.] UNPAUSE");

        if (input_get(&state->input, PAUSEKEYS) & INPUT_PRESS) {
            LOG("WHAT?");
            unpaused = true;
            state->paused = false;
        }

        if (input_get(&state->input, "space") & INPUT_PRESS) {
            state_set_stage(state, STAGE_MAIN_MENU);
            unpaused = true;
            state->paused = false;
        }
    } else {
        ui_update();

        const f32 dt = state->time.delta / 1000000000.0f;
        level_update(state->level, dt);
    }

    if (!unpaused
        && (state->stage == STAGE_PLAY
        || state->stage == STAGE_BUILD)) {
        if (input_get(&state->input, PAUSEKEYS) & INPUT_PRESS) {
            state->paused = true;
        }
    }

    for (u64 i = 0; i < state->time.frame_ticks; i++) {
        state->time.tick++;
        state->time.animtick = state->time.tick / 20;
        state->tick_bullet_sounds = 0;
        state->tick_sounds = 0;

        if (state->stage != STAGE_MAIN_MENU && !state->paused) {
            level_tick(state->level);

            // tick particles
            dynlist_each(state->particles, it) {
                particle_tick(it.el);

                if (it.el->delete) {
                    dynlist_remove_it(state->particles, it);
                }
            }
        }
    }

    int w, h;
    SDL_GL_GetDrawableSize(state->window, &w, &h);

    offscreen.passaction = (sg_pass_action) {
        .colors[0] = {
            .action = SG_ACTION_CLEAR,
            .value = { state->clear_color.r, state->clear_color.g, state->clear_color.b, 1.0f }
        },
    };

    sg_begin_pass(offscreen.pass, &offscreen.passaction);

    if (state->stage == STAGE_MAIN_MENU) {
        main_menu_draw(&state->_main_menu);
    } else if (state->stage == STAGE_TITLE) {
        title_draw();
    } else {
        state->clear_color = COLOR_BLACK;
        level_draw(state->level);

        // draw particles
#define MAX_DRAW_PARTICLES 1024
        dynlist_each(state->particles, it) {
            if (it.i >= MAX_DRAW_PARTICLES) { break; }
            particle_draw(it.el);
        }

        ui_draw();
    }

    const mat4s
        proj = glms_ortho(0.0f, TARGET_SIZE.x, 0.0f, TARGET_SIZE.y, Z_MIN, Z_MAX),
        view = glms_mat4_identity();

    gfx_batcher_draw(&state->batcher, &proj, &view);
    gfx_batcher_clear(&state->batcher);

    sg_end_pass();
    sg_commit();

    sg_pass_action pass_action = { 0 };
    pass_action.colors[0] = (sg_color_attachment_action) {
        .action = SG_ACTION_CLEAR,
        .value = { state->clear_color.r, state->clear_color.g, state->clear_color.b, 1.0f }
    };

    sg_begin_default_pass(&pass_action, w, h);
    gfx_screenquad(offscreen.color);
    sg_end_pass();
    sg_commit();

    SDL_GL_SwapWindow(state->window);
}

// see state->h
global_state *state;

int main(int argc, char *argv[]) {
    state = calloc(1, sizeof(*state));

    ASSERT(
        !SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO),
        "failed to init SDL: %s", SDL_GetError());

    ASSERT(
        IMG_Init(IMG_INIT_PNG) == IMG_INIT_PNG,
        "failed to init SDL_Image, %s",
        IMG_GetError());

    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    printf("Compiled SDL version: %d.%d.%d\n",
           compiled.major, compiled.minor, compiled.patch);
    printf("Linked SDL version: %d.%d.%d\n",
           linked.major, linked.minor, linked.patch);

    state->window =
        SDL_CreateWindow(
            "OUT",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            WINDOW_SIZE.x, WINDOW_SIZE.y,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    ASSERT(state->window);

#ifdef EMSCRIPTEN
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif // ifdef EMSCRIPTEN

    state->glctx = SDL_GL_CreateContext(state->window);
    ASSERT(state->glctx);

    ASSERT(!glGetError());

    printf("GL Version={%s}\n", glGetString(GL_VERSION));
    printf("GLSL Version={%s}\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    SDL_GL_MakeCurrent(state->window, state->glctx);
    SDL_GL_SetSwapInterval(0);

    init();

#ifdef EMSCRIPTEN
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (!state->quit) { frame(); }
#endif // ifdef EMSCRIPTEN

    deinit();

    SDL_GL_DeleteContext(state->glctx);
    SDL_DestroyWindow(state->window);
    SDL_Quit();

    return 0;
}
