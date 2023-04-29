#pragma once

#include <stdio.h>

#include <cjam/macros.h>
#include <cjam/math.h>

#define IVEC2S2V(_i) ({ ivec2s __i = (_i); (vec2s) {{ __i.x, __i.y }}; })
#define VEC2S2I(_i) ({ vec2s __i = (_i); (ivec2s) {{ __i.x, __i.y }}; })

#define GRAYSCALE(_x, _a) ({ TYPEOF(_x) __x = (_x); VEC4S(__x, __x, __x, (_a)); })

ALWAYS_INLINE int resource_to_path(char *dst, int n, const char *resource) {
    int res;

#ifdef EMSCRIPTEN
    res = snprintf(dst, n, "/res/%s", resource);
#else
    res = snprintf(dst, n, "res/%s", resource);
#endif // ifdef EMSCRIPTEN

    return !(res >= 0 && res <= n);
}