#pragma once

#ifndef SOKOL_GFX_INCLUDED

#ifdef EMSCRIPTEN
#   define SOKOL_GLES3
#else
#   define SOKOL_GLCORE33
#endif // ifdef EMSCRIPTEN

#include <sokol_gfx.h>

#endif // ifdef SOKOL_GFX_INCLUDED

#include <cjam/math.h>
#include <cjam/map.h>

enum {
    GFX_NO_FLAGS   = 0,
    GFX_FLIP_X     = 1 << 0,
    GFX_FLIP_Y     = 1 << 1,
    GFX_ROTATE_CW  = 1 << 2,
    GFX_ROTATE_CCW = 1 << 3,
};

typedef struct {
    sg_image image;
    ivec2s sprite_size, size_px, size_sprites;
    vec2s uv_unit;
} gfx_atlas;

typedef struct {
    vec2s pos;
    ivec2s index;
    vec4s color;
    f32 z;
    int flags;
} gfx_sprite;

typedef struct {
    // map of sg_image.id -> entry
    struct map image_lists;

    sg_buffer instance_data;
} gfx_batcher;

int gfx_load_image(const char *resource, sg_image *out);
void gfx_screenquad(sg_image image);

void gfx_atlas_init(gfx_atlas *atlas, sg_image image, ivec2s sprite_size);
void gfx_atlas_destroy(gfx_atlas *atlas);

void gfx_batcher_init(gfx_batcher *batcher);
void gfx_batcher_destroy(gfx_batcher *batcher);

// push sprite to render from atlas
void gfx_batcher_push_sprite(
    gfx_batcher *batcher,
    const gfx_atlas *atlas,
    const gfx_sprite *sprite);

void gfx_batcher_push_image(
    gfx_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z,
    int flags);

void gfx_batcher_push_subimage(
    gfx_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z,
    int flags,
    ivec2s offset,
    ivec2s size);

void gfx_batcher_draw(
    const gfx_batcher *batcher,
    const mat4s *proj,
    const mat4s *view);

void gfx_batcher_clear(gfx_batcher *batcher);
