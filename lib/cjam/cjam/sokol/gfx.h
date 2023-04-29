#pragma once

#ifndef SOKOL_GFX_INCLUDED

#ifdef EMSCRIPTEN
#   define SOKOL_GLES3
#else
#   define SOKOL_GLCORE33
#endif // ifdef EMSCRIPTEN

#include <sokol_gfx.h>

#endif // ifdef SOKOL_GFX_INCLUDED

#include "../config.h"
#include "../math.h"
#include "../map.h"
#include "../dynlist.h"

typedef struct {
    sg_image image;
    ivec2s sprite_size, size_px, size_sprites;
    vec2s uv_unit;
} cj_atlas;

typedef struct {
    vec2s pos;
    ivec2s index;
    vec4s color;
    f32 z;
} cj_sprite;

typedef struct {
    // map of sg_image.id -> entry
    struct map image_lists;

    sg_buffer instance_data;
} cj_batcher;

void cj_atlas_init(cj_atlas *atlas, sg_image image, ivec2s sprite_size);
void cj_atlas_destroy(cj_atlas *atlas);

void cj_batcher_init(cj_batcher *batcher);
void cj_batcher_destroy(cj_batcher *batcher);

// push sprite to render from atlas
void cj_batcher_push_sprite(
    cj_batcher *batcher,
    const cj_atlas *atlas,
    const cj_sprite *sprite);

void cj_batcher_push_image(
    cj_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z);

void cj_batcher_push_subimage(
    cj_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z,
    ivec2s offset,
    ivec2s size);

void cj_batcher_draw(
    const cj_batcher *batcher,
    const mat4s *proj,
    const mat4s *view);

void cj_batcher_clear(cj_batcher *batcher);

#ifdef CJAM_IMPL

#include "batch.glsl.h"

void cj_atlas_init(cj_atlas *atlas, sg_image image, ivec2s sprite_size) {
    sg_image_desc imdesc = sg_query_image_desc(image);
    atlas->image = image;
    atlas->sprite_size = sprite_size;
    atlas->size_px = (ivec2s) {{ imdesc.width, imdesc.height }};
    atlas->size_sprites = (ivec2s) {{
        atlas->size_px.x / atlas->sprite_size.x,
        atlas->size_px.y / atlas->sprite_size.y,
    }};
    atlas->uv_unit = (vec2s) {{
        1.0f / atlas->size_sprites.x,
        1.0f / atlas->size_sprites.y
    }};
}

void cj_atlas_destroy(cj_atlas *atlas) {
    sg_destroy_image(atlas->image);
}

typedef struct {
    vec2s offset;
    vec2s scale;
    vec2s uv_min;
    vec2s uv_max;
    vec4s color;
    f32 z;
} cj_batcher_entry;

typedef struct {
    sg_image image;
    DYNLIST(cj_batcher_entry) entries;
    int data_offset;
} cj_batcher_list;

static struct {
    bool init;
    sg_shader shader;
    sg_pipeline pipeline;
    sg_buffer indices, vertices;
} g_batcher;

static void atlaslists_valfree(cj_batcher_list *l, void*) {
    dynlist_free(l->entries);
    CJAM_FREE(l);
}

void cj_batcher_init(cj_batcher *batcher) {
    if (!g_batcher.init) {
        g_batcher.init = true;

        static const f32 vertices[] = {
            // pos        // uv
            0.0f, 1.0f,  0.0f, 1.0f,
            1.0f, 1.0f,  1.0f, 1.0f,
            1.0f, 0.0f,   1.0f, 0.0f,
            0.0f, 0.0f,   0.0f, 0.0f,
        };

        static const u16 indices[] = {
            0, 1, 2,
            0, 2, 3
        };

        // TODO: add shutdown hook
        g_batcher.vertices =
            sg_make_buffer(&(sg_buffer_desc) { .data = SG_RANGE(vertices) });
        g_batcher.indices =
            sg_make_buffer(
                &(sg_buffer_desc) {
                    .type = SG_BUFFERTYPE_INDEXBUFFER,
                    .data = SG_RANGE(indices)
                });

        g_batcher.shader =
            sg_make_shader(batch_batch_shader_desc(sg_query_backend()));

        g_batcher.pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
            .shader = g_batcher.shader,
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
            .index_type = SG_INDEXTYPE_UINT16,
            .layout = {
                .buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE,
                .buffers[1].stride = sizeof(cj_batcher_entry),
                .attrs = {
                    [ATTR_batch_vs_a_position] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .buffer_index = 0,
                    },
                    [ATTR_batch_vs_a_texcoord0] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .buffer_index = 0,
                    },
                    [ATTR_batch_vs_a_offset] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .offset = offsetof(cj_batcher_entry, offset),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_scale] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .offset = offsetof(cj_batcher_entry, scale),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_uvmin] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .offset = offsetof(cj_batcher_entry, uv_min),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_uvmax] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .offset = offsetof(cj_batcher_entry, uv_max),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_color] = {
                        .format = SG_VERTEXFORMAT_FLOAT4,
                        .offset = offsetof(cj_batcher_entry, color),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_z] = {
                        .format = SG_VERTEXFORMAT_FLOAT,
                        .offset = offsetof(cj_batcher_entry, z),
                        .buffer_index = 1,
                    },
                }
            },
            .colors[0].blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_rgb = SG_BLENDOP_ADD,
                .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_alpha = SG_BLENDOP_ADD,
            },
            .depth = {
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true,
            },
            .cull_mode = SG_CULLMODE_BACK,
        });
    }

    map_init(
        &batcher->image_lists,
        map_hash_id,
        NULL,
        NULL,
        NULL,
        map_cmp_id,
        NULL,
        (f_map_free) atlaslists_valfree,
        NULL);

    batcher->instance_data =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .size = 8 * 1024 * 1024,
                .usage = SG_USAGE_STREAM
            });
}

void cj_batcher_destroy(cj_batcher *batcher) {
    map_destroy(&batcher->image_lists);
    sg_destroy_buffer(batcher->instance_data);
}

static cj_batcher_list *list_for_image(cj_batcher *batcher, sg_image image) {
    cj_batcher_list
        **pslot = map_find(cj_batcher_list*, &batcher->image_lists, image.id),
        *list;

    if (pslot) {
        list = *pslot;
    } else {
        list = CJAM_ALLOC(sizeof(*list) /* NOLINT */, NULL);
        memset(list, 0, sizeof(*list) /* NOLINT */);
        list->image = image;
        map_insert(&batcher->image_lists, image.id, list);
    }

    return list;
}

// push sprite to render from atlas
void cj_batcher_push_sprite(
    cj_batcher *batcher,
    const cj_atlas *atlas,
    const cj_sprite *sprite) {
    cj_batcher_list *list = list_for_image(batcher, atlas->image);
    *dynlist_push(list->entries) = (cj_batcher_entry) {
        .offset = sprite->pos,
        .scale = {{
            atlas->sprite_size.x,
            atlas->sprite_size.y,
        }},
        .uv_min = {{
            (sprite->index.x + 0) * atlas->uv_unit.x,
            (sprite->index.y + 0) * atlas->uv_unit.y,
        }},
        .uv_max = {{
            (sprite->index.x + 1) * atlas->uv_unit.x,
            (sprite->index.y + 1) * atlas->uv_unit.y,
        }},
        .color = sprite->color,
        .z = sprite->z
    };
}

void cj_batcher_push_image(
    cj_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z) {
    sg_image_desc desc = sg_query_image_desc(image);
    cj_batcher_push_subimage(
        batcher, image, pos, color, z,
        (ivec2s) {{ 0, 0 }},
        (ivec2s) {{ desc.width, desc.height }});
}

void cj_batcher_push_subimage(
    cj_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z,
    ivec2s offset,
    ivec2s size) {
    sg_image_desc desc = sg_query_image_desc(image);
    cj_batcher_list *list = list_for_image(batcher, image);
    const vec2s uv_unit = {{
        1.0f / desc.width,
        1.0f / desc.height
    }};
    *dynlist_push(list->entries) = (cj_batcher_entry) {
        .offset = pos,
        .scale = {{ size.x, size.y }},
        .uv_min = {{
            offset.x * uv_unit.x,
            offset.y * uv_unit.y,
        }},
        .uv_max = {{
            (offset.x + (size.x - 1)) * uv_unit.x,
            (offset.y + (size.y - 1)) * uv_unit.y,
        }},
        .color = color,
        .z = z
    };
}

void cj_batcher_draw(
    const cj_batcher *batcher,
    const mat4s *proj,
    const mat4s *view) {
    // accumulate total number of sprites to draw
    int n = 0;
    map_each(u32, cj_batcher_list*, &batcher->image_lists, it) {
        n += dynlist_size(it.value->entries);
    }

    if (n == 0) { return; }

    // process sprites into buffer data
    int i = 0;
    cj_batcher_entry *entries = CJAM_ALLOC(n * sizeof(cj_batcher_entry), NULL);

    map_each(u32, cj_batcher_list*, &batcher->image_lists, it_map) {
        cj_batcher_list *list = it_map.value;
        if (dynlist_size(list->entries) == 0) {
            continue;
        }
        list->data_offset = i * sizeof(cj_batcher_entry);
        dynlist_each(list->entries , it) {
            entries[i++] = *it.el;
        }
    }

    ASSERT(i == n);
    sg_update_buffer(
        batcher->instance_data, &(sg_range) { entries, n * sizeof(*entries) });
    ASSERT(!sg_query_buffer_overflow(batcher->instance_data));

    sg_apply_pipeline(g_batcher.pipeline);

    // draw for each atlas
    map_each(u32, cj_batcher_list*, &batcher->image_lists, it_map) {
        cj_batcher_list *list = it_map.value;
        if (dynlist_size(list->entries) == 0) {
            continue;
        }

        const sg_bindings bind = {
            .fs_images[0] = list->image,
            .vertex_buffers[0] = g_batcher.vertices,
            .vertex_buffers[1] = batcher->instance_data,
            .vertex_buffer_offsets[1] = list->data_offset,
            .index_buffer = g_batcher.indices
        };

        const mat4s model = glms_mat4_identity();

        batch_vs_params_t vsparams;
        memcpy(vsparams.model, &model, sizeof(model));
        memcpy(vsparams.view, view, sizeof(*view));
        memcpy(vsparams.proj, proj, sizeof(*proj));

        sg_apply_bindings(&bind);
        sg_apply_uniforms(
            SG_SHADERSTAGE_VS, SLOT_batch_vs_params, SG_RANGE_REF(vsparams));
        sg_draw(0, 6, dynlist_size(list->entries));
    }

    CJAM_FREE(entries);
}

void cj_batcher_clear(cj_batcher *batcher) {
    map_clear(&batcher->image_lists);
}

#endif // CJAM_IMPL
