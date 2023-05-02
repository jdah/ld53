#include "gfx.h"
#include "util.h"
#include "sdl.h"

#include <cjam/dynlist.h>
#include <cjam/file.h>

#define SOKOL_SHDC_IMPL
#include "shader/batch.glsl.h"
#include "shader/basic.glsl.h"

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

int gfx_load_image(const char *resource, sg_image *out) {
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

void gfx_screenquad(sg_image image) {
    static bool init;
    static sg_pipeline pipeline;
    static sg_shader shader;

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

    static sg_buffer vbuf, ibuf;

    if (!init) {
        init = true;

        vbuf = sg_make_buffer(&(sg_buffer_desc) { .data = SG_RANGE(vertices) });
        ibuf =
            sg_make_buffer(
                &(sg_buffer_desc) {
                    .type = SG_BUFFERTYPE_INDEXBUFFER,
                    .data = SG_RANGE(indices)
                });

        shader = sg_make_shader(basic_basic_shader_desc(sg_query_backend()));

        pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
            .shader = shader,
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
            .index_type = SG_INDEXTYPE_UINT16,
            .layout = {
                .attrs = {
                    [0].format = SG_VERTEXFORMAT_FLOAT2,
                    [1].format = SG_VERTEXFORMAT_FLOAT2,
                }
            },
            .depth = {
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true
            },
            .cull_mode = SG_CULLMODE_BACK,
        });
    }

    const sg_bindings bind = {
        .fs_images[0] = image,
        .vertex_buffers[0] = vbuf,
        .index_buffer = ibuf
    };

    const mat4s
        model = glms_mat4_identity(),
        view = glms_mat4_identity(),
        proj = glms_ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

    basic_vs_params_t vsparams;
    memcpy(vsparams.model, &model, sizeof(model));
    memcpy(vsparams.view, &view, sizeof(view));
    memcpy(vsparams.proj, &proj, sizeof(proj));

    sg_apply_pipeline(pipeline);
    sg_apply_bindings(&bind);
    sg_apply_uniforms(
        SG_SHADERSTAGE_VS, SLOT_basic_vs_params, SG_RANGE_REF(vsparams));
    sg_draw(0, 6, 1);
}

void gfx_atlas_init(gfx_atlas *atlas, sg_image image, ivec2s sprite_size) {
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

void gfx_atlas_destroy(gfx_atlas *atlas) {
    sg_destroy_image(atlas->image);
}

typedef struct {
    vec2s offset;
    vec2s scale;
    vec2s uv_min;
    vec2s uv_max;
    vec4s color;
    f32 z;
    f32 flags;
} gfx_batcher_entry;

typedef struct {
    sg_image image;
    DYNLIST(gfx_batcher_entry) entries;
    int data_offset;
} gfx_batcher_list;

static struct {
    bool init;
    sg_shader shader;
    sg_pipeline pipeline;
    sg_buffer indices, vertices;
} g_batcher;

static void atlaslists_valfree(gfx_batcher_list *l, void*) {
    dynlist_free(l->entries);
    CJAM_FREE(l);
}

void gfx_batcher_init(gfx_batcher *batcher) {
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
                .buffers[1].stride = sizeof(gfx_batcher_entry),
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
                        .offset = offsetof(gfx_batcher_entry, offset),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_scale] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .offset = offsetof(gfx_batcher_entry, scale),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_uvmin] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .offset = offsetof(gfx_batcher_entry, uv_min),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_uvmax] = {
                        .format = SG_VERTEXFORMAT_FLOAT2,
                        .offset = offsetof(gfx_batcher_entry, uv_max),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_color] = {
                        .format = SG_VERTEXFORMAT_FLOAT4,
                        .offset = offsetof(gfx_batcher_entry, color),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_z] = {
                        .format = SG_VERTEXFORMAT_FLOAT,
                        .offset = offsetof(gfx_batcher_entry, z),
                        .buffer_index = 1,
                    },
                    [ATTR_batch_vs_a_flags] = {
                        .format = SG_VERTEXFORMAT_FLOAT,
                        .offset = offsetof(gfx_batcher_entry, flags),
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
                .size = 64 * 1024 * 1024,
                .usage = SG_USAGE_STREAM
            });
}

void gfx_batcher_destroy(gfx_batcher *batcher) {
    map_destroy(&batcher->image_lists);
    sg_destroy_buffer(batcher->instance_data);
}

static gfx_batcher_list *list_for_image(gfx_batcher *batcher, sg_image image) {
    gfx_batcher_list
        **pslot = map_find(gfx_batcher_list*, &batcher->image_lists, image.id),
        *list;

    if (pslot) {
        list = *pslot;
    } else {
        LOG("new list for image %d", image.id);
        list = CJAM_ALLOC(sizeof(*list) /* NOLINT */, NULL);
        memset(list, 0, sizeof(*list) /* NOLINT */);
        list->image = image;
        map_insert(&batcher->image_lists, image.id, list);
    }

    return list;
}

// push sprite to render from atlas
void gfx_batcher_push_sprite(
    gfx_batcher *batcher,
    const gfx_atlas *atlas,
    const gfx_sprite *sprite) {
    gfx_batcher_list *list = list_for_image(batcher, atlas->image);
    // TODO ??
    /* const vec2s half_px = {{ */
    /*     (1.0f / atlas->size_px.x) / 8.0f, */
    /*     (1.0f / atlas->size_px.y) / 8.0f, */
    /* }}; */
    *dynlist_push(list->entries) = (gfx_batcher_entry) {
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
        .z = sprite->z,
        .flags = sprite->flags
    };
}

void gfx_batcher_push_image(
    gfx_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z,
    int flags) {
    sg_image_desc desc = sg_query_image_desc(image);
    gfx_batcher_push_subimage(
        batcher, image, pos, color, z, flags,
        (ivec2s) {{ 0, 0 }},
        (ivec2s) {{ desc.width, desc.height }});
}

void gfx_batcher_push_subimage(
    gfx_batcher *batcher,
    sg_image image,
    vec2s pos,
    vec4s color,
    f32 z,
    int flags,
    ivec2s offset,
    ivec2s size) {
    sg_image_desc desc = sg_query_image_desc(image);
    gfx_batcher_list *list = list_for_image(batcher, image);
    const vec2s uv_unit = {{
        1.0f / desc.width,
        1.0f / desc.height
    }};
    *dynlist_push(list->entries) = (gfx_batcher_entry) {
        .offset = pos,
        .scale = {{ size.x, size.y }},
        .uv_min = {{
            offset.x * uv_unit.x,
            offset.y * uv_unit.y,
        }},
        .uv_max = {{
            (offset.x + (size.x - 0)) * uv_unit.x,
            (offset.y + (size.y - 0)) * uv_unit.y,
        }},
        .color = color,
        .z = z,
        .flags = flags
    };
}

void gfx_batcher_draw(
    const gfx_batcher *batcher,
    const mat4s *proj,
    const mat4s *view) {
    // accumulate total number of sprites to draw
    int n = 0;
    map_each(u32, gfx_batcher_list*, &batcher->image_lists, it) {
        n += dynlist_size(it.value->entries);
    }

    if (n == 0) { return; }

    // process sprites into buffer data
    int i = 0;
    gfx_batcher_entry *entries = CJAM_ALLOC(n * sizeof(gfx_batcher_entry), NULL);

    map_each(u32, gfx_batcher_list*, &batcher->image_lists, it_map) {
        gfx_batcher_list *list = it_map.value;
        if (dynlist_size(list->entries) == 0) {
            continue;
        }
        list->data_offset = i * sizeof(gfx_batcher_entry);
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
    map_each(u32, gfx_batcher_list*, &batcher->image_lists, it_map) {
        gfx_batcher_list *list = it_map.value;
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
        dynlist_free(list->entries);
    }

    CJAM_FREE(entries);
}

void gfx_batcher_clear(gfx_batcher *batcher) {
    /* map_clear(&batcher->image_lists); */
}
