#include <stdio.h>

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

struct {
    bool quit;
    SDL_Window *window;
    SDL_GLContext *glctx;
    input input;
} state;

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

static int resource_to_path(char *dst, int n, const char *resource) {
    int res;

#ifdef EMSCRIPTEN
    res = snprintf(dst, n, "/res/%s", resource);
#else
    res = snprintf(dst, n, "res/%s", resource);
#endif // ifdef EMSCRIPTEN

    return !(res >= 0 && res <= n);
}

static int load_image(const char *resource, u8 **pdata, ivec2s *psize) {
    char filepath[1024];
    resource_to_path(filepath, sizeof(filepath), resource);

    char *filedata;
    usize filesz;
    ASSERT(
        !file_read(filepath, &filedata, &filesz),
        "failed to read %s",
        filepath);

    SDL_RWops *rw = SDL_RWFromMem(filedata, filesz);
    ASSERT(rw);

    SDL_Surface *surf = IMG_LoadPNG_RW(rw);
    ASSERT(surf);
    ASSERT(surf->w >= 0 && surf->h >= 0);

    if (surf->format->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *newsurf =
            SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surf);
        surf = newsurf;
    }

    *pdata = malloc(surf->w * surf->h * 4);
    *psize = (ivec2s) {{ surf->w, surf->h }};

    for (int y = 0; y < surf->h; y++) {
        memcpy(
            &(*pdata)[y * (psize->x * 4)],
            &((u8*) surf->pixels)[(surf->h - y - 1) * (psize->x * 4)],
            psize->x * 4);
    }

    SDL_FreeSurface(surf);
    SDL_FreeRW(rw);
    CJAM_FREE(filedata);

    return 0;
}

static int load_sg_image(const char *resource, sg_image *out) {
    int res;
    u8 *data;
    ivec2s size;

    if ((res = load_image(resource, &data, &size))) {
        return res;
    }

    *out =
        sg_make_image(
            &(sg_image_desc) {
                .width = size.x,
                .height = size.y,
                .data.subimage[0][0] = {
                    .ptr = data,
                    .size = (size_t) (size.x * size.y * 4),
                }
            });
    free(data);

    return 0;
}

sg_image g_image;

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

    input_init(&state.input);

    sg_setup(&(sg_desc) {
        .logger = (sg_logger) { .func = _sg_logger }
    });
    assert(sg_isvalid());

    const ivec2s targetsize = {{ 400, 300 }};

    offscreen.color =
        sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = targetsize.x,
                .height = targetsize.y,
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
                .width = targetsize.x,
                .height = targetsize.y,
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
            .value = { 1.0f, 0.0f, 1.0f, 1.0f }
        },
        /* .depth = { */
        /*     .action = SG_ACTION_CLEAR, */
        /*     .value = 1000.0f */
        /* } */
    };

    u8 *imdata;
    ivec2s imsize;
    ASSERT(!load_image("font.png", &imdata, &imsize));

    g_image = sg_make_image(&(sg_image_desc) {
        .width = imsize.x,
        .height = imsize.y,
        .data.subimage[0][0] = {
            .ptr = imdata,
            .size = (size_t) (imsize.x * imsize.y * 4),
        }
    });
    free(imdata);
}

static void frame() {
    input_update(&state.input);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            state.quit = true;
            break;
        }

        input_process(&state.input, &event);
    }

    static bool init;
    static gfx_atlas atlas;
    static gfx_batcher batcher;

    if (!init) {
        init = true;
        gfx_batcher_init(&batcher);

        sg_image image;
        ASSERT(!load_sg_image("font.png", &image));
        gfx_atlas_init(&atlas, image, (ivec2s) {{ 8, 8 }});
    }

    int w, h;
    SDL_GL_GetDrawableSize(state.window, &w, &h);

    sg_begin_pass(offscreen.pass, &offscreen.passaction);

    const f32 time = time_ns() / 1000000.0f;
    struct rand rand = rand_create(0x12345);

    for (int i = 0; i < 128; i++) {
        const f32
            r = rand_f64(&rand, 20.0f, 30.0f),
            t = rand_f64(&rand, 0.0f, TAU);

        gfx_batcher_push_sprite(
            &batcher,
            &atlas,
            &(gfx_sprite) {
                .index = {{ i % 16, 11 }},
                .pos = {{ 40 + r * cos(time + t), 40 + r * sin(time + t) }},
                .color = {{ 1.0f, 1.0f, 1.0f, 1.0f }},
                .z = 0.001f
            });

    }

    const mat4s
        proj = glms_ortho(0.0f, 200.0f, 0.0f, 150.0f, -10.0f, 10.0f),
        view = glms_mat4_identity();

    gfx_batcher_draw(&batcher, &proj, &view);
    gfx_batcher_clear(&batcher);

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

    SDL_GL_SwapWindow(state.window);
}

int main(int argc, char *argv[]) {
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

    state.window =
        SDL_CreateWindow(
            "OUT",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            800, 600,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    ASSERT(state.window);

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

    state.glctx = SDL_GL_CreateContext(state.window);
    ASSERT(state.glctx);

    ASSERT(!glGetError());

    printf("GL Version={%s}\n", glGetString(GL_VERSION));
    printf("GLSL Version={%s}\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    SDL_GL_MakeCurrent(state.window, state.glctx);
    SDL_GL_SetSwapInterval(0);

    init();

#ifdef EMSCRIPTEN
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (!state.quit) { frame(); }
#endif // ifdef EMSCRIPTEN

    SDL_GL_DeleteContext(state.glctx);
    SDL_DestroyWindow(state.window);
    SDL_Quit();

    return 0;
}
