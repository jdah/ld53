#pragma once

#include <stddef.h>

// TODO
// feature-select macros, either uncomment here or where cjam headers are used
#define CJAM_SDL

// platform macros
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#   define CJAM_WINDOWS
#elifdef __APPLE__
#   define CJAM_OSX
#elifdef __linux__
#   define CJAM_LINUX
#elifdef EMSCRIPTEN
#   define CJAM_EMSCRIPTEN
#else
#   error "unknown platform"
#endif

#ifdef CJAM_EMSCRIPTEN
#include <emscripten.h>
#endif // ifdef CJAM_EMSCRIPTEN

#ifdef CJAM_SDL
#include <SDL.h>
#endif // ifdef CJAM_SDL

// CJAM memory interface
//
// allocates specified number of bytes, realloc'ing old ptr if present
// CJAM_ALLOC(number of bytes, old ptr (if realloc))
//
// // frees memory allocated by CJAM_ALLOC
// CJAM_FREE(ptr)

#ifdef CJAM_ALLOC
#   ifndef CJAM_FREE
#       error "CJAM_ALLOC defined but not CJAM_FREE"
#   endif
#else
void *_cjam_builtin_alloc(size_t, void*);
#define CJAM_ALLOC _cjam_builtin_alloc

void _cjam_builtin_free(void*);
#define CJAM_FREE _cjam_builtin_free
#endif // ifdef CJAM_ALLOC

#ifdef CJAM_IMPL
#include <stdlib.h>

void *_cjam_builtin_alloc(size_t n, void *p) {
    return realloc(p, n);
}

void _cjam_builtin_free(void *p) {
    free(p);
}
#endif // ifdef CJAM_IMPL
