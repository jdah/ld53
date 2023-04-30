#pragma once

#include <cjam/math.h>

void ui_update();

void ui_draw();

void ui_set_desc(const char *fmt, ...);

int ui_center_str(
    int y,
    f32 z,
    vec4s color,
    const char *fmt,
    ...);
