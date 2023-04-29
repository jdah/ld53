#include "defs.h"
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

#define CJAM_IMPL
#include <cjam/dynlist.h>
#include <cjam/file.h>
#include <cjam/rand.h>
#include <cjam/sound.h>
#include <cjam/time.h>
#include <cjam/log.h>

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

    offscreen.passaction = (sg_pass_action) {
        .colors[0] = {
            .action = SG_ACTION_CLEAR,
            .value = { 0.0f, 0.0f, 0.0f, 1.0f }
        },
    };

    gfx_batcher_init(&state->batcher);

    sg_image font_image;
    ASSERT(!gfx_load_image("font.png", &font_image));
    gfx_atlas_init(&state->atlas.font, font_image, (ivec2s) {{ 8, 8 }});

    sg_image tile_image;
    ASSERT(!gfx_load_image("tile.png", &tile_image));
    gfx_atlas_init(&state->atlas.tile, tile_image, (ivec2s) {{ 8, 8 }});

    // TODO: multiple levels
    state->level = calloc(1, sizeof(*state->level));
    level_init(state->level);
}

static void deinit() {
    gfx_atlas_destroy(&state->atlas.font);
    gfx_atlas_destroy(&state->atlas.tile);
    gfx_batcher_destroy(&state->batcher);
}

static void frame() {
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

    int w, h;
    SDL_GL_GetDrawableSize(state->window, &w, &h);

    sg_begin_pass(offscreen.pass, &offscreen.passaction);

    const f32 time = time_ns() / 1000000.0f;
    struct rand rand = rand_create(0x12345);

    for (int i = 0; i < 16; i++) {
        const f32
            r = rand_f64(&rand, 20.0f, 30.0f),
            t = rand_f64(&rand, 0.0f, TAU);

        gfx_batcher_push_sprite(
            &state->batcher,
            &state->atlas.font,
            &(gfx_sprite) {
                .index = {{ i % 16, 11 }},
                .pos = {{ 40 + r * cos(time + t), 40 + r * sin(time + t) }},
                .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
                .z = Z_UI,
                .flags = GFX_NO_FLAGS
            });

    }

    level_draw(state->level);

    font_str(
        state->input.cursor.pos,
        Z_UI,
        COLOR_WHITE,
        FONT_DOUBLED,
        "HELLO, $11WORLD!");

    const ivec2s cursor_tile = level_px_to_tile(state->input.cursor.pos);
    const ivec2s cursor_tile_px =
        level_px_round_to_tile(state->input.cursor.pos);
    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.tile,
        &(gfx_sprite) {
            .index = {{ 0, 15 }},
            .pos = {{ cursor_tile_px.x, cursor_tile_px.y }},
            .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
            .z = Z_UI,
            .flags = GFX_NO_FLAGS
        });

    if (input_get(&state->input, "mouse_left") & INPUT_PRESS) {
        state->level->objects[cursor_tile.x][cursor_tile.y] = OBJECT_TURRET_L0;
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
        .value = { 0.0f, 0.0f, 0.0f, 1.0f }
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
