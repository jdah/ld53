#pragma once

#include <cjam/types.h>
#include <cjam/math.h>

enum {
    FONT_DEFAULT  = 0,
    FONT_DOUBLED  = 1 << 0,
};

void font_char(
    ivec2s pos,
    f32 z,
    vec4s col,
    int flags,
    char c);

void font_str(
    ivec2s pos,
    f32 z,
    vec4s col,
    int flags,
    const char *str);

void font_v(
    ivec2s pos,
    f32 z,
    vec4s col,
    int flags,
    const char *fmt,
    ...);

int font_width(const char *str);

int font_len(const char *str);
