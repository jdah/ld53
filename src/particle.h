#pragma once

#include "defs.h"

#include <cjam/math.h>

typedef struct {
    particle_type type;
    int lifetime, ticks;
    vec2s pos, vel, drag, gravity, start_pos;
    vec4s color;
    bool delete, started;

    union {
        struct {
            char str[256];
        } text;
    };
} particle;

particle *particle_new_text(
    vec2s pos,
    vec4s color,
    int ticks,
    const char *fmt,
    ...);

particle *particle_new_smoke(
    vec2s pos,
    vec4s color,
    int ticks);

particle *particle_new_splat(
    vec2s pos,
    vec4s color,
    int ticks);

void particle_new_multi_splat(
    vec2s pos,
    vec4s color,
    int ticks,
    int mi,
    int ma);

void particle_tick(particle*);
void particle_draw(particle*);
