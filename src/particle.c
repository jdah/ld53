#include "particle.h"
#include "cjam/rand.h"
#include "font.h"
#include "state.h"
#include "util.h"

#include <cjam/dynlist.h>
#include <stdarg.h>

particle *particle_new_text(
    vec2s pos,
    vec4s color,
    int ticks,
    const char *fmt,
    ...) {
    va_list ap;
    va_start(ap);
    particle p = {
        .type = PARTICLE_TEXT,
        .lifetime = ticks,
        .ticks = ticks,
        .pos = pos,
        .vel = VEC2S(0.0f, 0.15f),
        .drag = VEC2S(1.0f),
        .color = color,
    };
    vsnprintf(p.text.str, sizeof(p.text.str), fmt, ap);
    va_end(ap);

    particle *res = dynlist_push(state->particles);
    *res = p;
    return res;
}

particle *particle_new_smoke(
    vec2s pos,
    vec4s color,
    int ticks) {
    struct rand r = rand_create(pos.x * pos.y + state->time.tick);
    particle *res;
    *(res = dynlist_push(state->particles)) = (particle) {
        .type = PARTICLE_PIXEL,
        .lifetime = ticks,
        .ticks = ticks,
        .pos = pos,
        .color = color,
        .vel = VEC2S(rand_f64(&r, -0.2f, 0.2f), rand_f64(&r, 1.0f, 1.3f)),
        .drag = VEC2S(0.9f),
    };
    return res;
}

particle *particle_new_splat(
    vec2s pos,
    vec4s color,
    int ticks) {
    struct rand r = rand_create(pos.x * pos.y + state->time.tick + ticks);
    particle *res;
    *(res = dynlist_push(state->particles)) = (particle) {
        .type = PARTICLE_PIXEL,
        .lifetime = ticks,
        .ticks = ticks,
        .pos = pos,
        .color = color,
        .vel = VEC2S(rand_f64(&r, -0.8f, 0.8f), rand_f64(&r, 1.0f, 1.8f)),
        .gravity = VEC2S(0.0f, -0.4f),
        .drag = VEC2S(0.9f, 0.85f),
    };
    return res;
}

void particle_new_multi_splat(
    vec2s pos,
    vec4s color,
    int ticks,
    int mi,
    int ma) {
    struct rand r =
        rand_create(pos.x * pos.y + ticks + mi + mi + state->time.tick);
    const int n = rand_n(&r, mi, ma);
    for (int i = 0; i < n; i++) {
        particle_new_splat(pos, color, ticks + rand_n(&r, -10, 10));
    }
}

void particle_tick(particle *p) {
    if (!p->started) {
        p->started = true;
        p->start_pos = p->pos;
    }

    p->ticks = max(p->ticks - 1, 0);

    if (p->ticks <= 0) {
        p->delete = true;
        return;
    }

    p->pos = glms_vec2_add(p->pos, p->vel);
    p->vel = glms_vec2_add(p->vel, p->gravity);
    p->vel = glms_vec2_mul(p->vel, p->drag);

    switch (p->type) {
    case PARTICLE_PIXEL: {
        if (p->pos.y < p->start_pos.y) { p->vel.y *= -0.75f; }
    } break;
    default: break;
    };
}

void particle_draw(particle *p) {
    switch (p->type) {
    case PARTICLE_TEXT: {
        font_str(
            VEC2S2I(p->pos),
            Z_UI, // TODO
            (vec4s) {{
                p->color.r, p->color.g, p->color.b,
                clamp(p->color.a * (p->ticks / (f32) (p->lifetime)), 0.0f, 1.0f),
            }},
            FONT_DOUBLED,
            p->text.str);
    } break;
    case PARTICLE_PIXEL: {
        gfx_batcher_push_sprite(
            &state->batcher,
            &state->atlas.tile,
            &(gfx_sprite) {
                .pos = p->pos,
                .index = {{ 1, 7 }},
                .color = p->color,
                .z = Z_LEVEL_ENTITY_OVERLAY - 0.001f, // TODO
                .flags = GFX_NO_FLAGS
            });
    } break;
    default: return;
    }
}
