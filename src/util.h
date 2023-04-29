#pragma once

#include <stdio.h>

#include <cjam/macros.h>

ALWAYS_INLINE int resource_to_path(char *dst, int n, const char *resource) {
    int res;

#ifdef EMSCRIPTEN
    res = snprintf(dst, n, "/res/%s", resource);
#else
    res = snprintf(dst, n, "res/%s", resource);
#endif // ifdef EMSCRIPTEN

    return !(res >= 0 && res <= n);
}
