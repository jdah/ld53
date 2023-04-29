#pragma once

#include <stdlib.h>

#include "log.h"

// assert _e, otherwise print _f with fmt __VA_ARGS__
#define _ASSERT_IMPL(_e, _fmt, ...)       \
    do {                                  \
        if (!(_e)) {                      \
            ERROR((_fmt), ##__VA_ARGS__); \
            DUMPTRACE();                  \
            exit(1);                      \
        }                                 \
    } while (0)

#define _ASSERT_GET(_0, _1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define _ASSERT0() _Pragma("error")
#define _ASSERT1(_e) _ASSERT_IMPL(_e, "%s", "assertion failed")
#define _ASSERT2(...) _ASSERT_IMPL(__VA_ARGS__)
#define _ASSERT3(...) _ASSERT_IMPL(__VA_ARGS__)
#define _ASSERT4(...) _ASSERT_IMPL(__VA_ARGS__)
#define _ASSERT5(...) _ASSERT_IMPL(__VA_ARGS__)
#define _ASSERT6(...) _ASSERT_IMPL(__VA_ARGS__)
#define _ASSERT7(...) _ASSERT_IMPL(__VA_ARGS__)
#define ASSERT(...)                         \
    _ASSERT_GET(                            \
        _0,                                 \
       ##__VA_ARGS__,                       \
        _ASSERT8,                           \
        _ASSERT7,                           \
        _ASSERT6,                           \
        _ASSERT5,                           \
        _ASSERT4,                           \
        _ASSERT3,                           \
        _ASSERT2,                           \
        _ASSERT1,                           \
        _ASSERT0)(__VA_ARGS__)

// assert _v and evaluate to its value
#define _VASSERT_IMPL(_v, _fmt, ...) ({     \
        TYPEOF(_v) __v = (_v);              \
        ASSERT(__v, (_fmt), ##__VA_ARGS__); \
        __v;                                \
    })

#define _VASSERT_GET(_0, _1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define _VASSERT0() _Pragma("error")
#define _VASSERT1(_e) _VASSERT_IMPL(_e, "%s", "assertion failed")
#define _VASSERT2(...) _VASSERT_IMPL(__VA_ARGS__)
#define _VASSERT3(...) _VASSERT_IMPL(__VA_ARGS__)
#define _VASSERT4(...) _VASSERT_IMPL(__VA_ARGS__)
#define _VASSERT5(...) _VASSERT_IMPL(__VA_ARGS__)
#define _VASSERT6(...) _VASSERT_IMPL(__VA_ARGS__)
#define _VASSERT7(...) _VASSERT_IMPL(__VA_ARGS__)
#define VASSERT(...)                        \
    _VASSERT_GET(                           \
        _0,                                 \
       ##__VA_ARGS__,                       \
        _VASSERT8,                          \
        _VASSERT7,                          \
        _VASSERT6,                          \
        _VASSERT5,                          \
        _VASSERT4,                          \
        _VASSERT3,                          \
        _VASSERT2,                          \
        _VASSERT1,                          \
        _VASSERT0)(__VA_ARGS__)

// define DUMPTRACE, which dumps a stacktrace to stdout on glibc targets
#ifdef __GLIBC__
#include <execinfo.h>
#define DUMPTRACE() do {                                   \
        void *callstack[128];                              \
        int frames = backtrace(callstack, 128);            \
        char **strs = backtrace_symbols(callstack, frames);\
        for (int i = 0; i < frames; i++) {                 \
            printf("%s\n", strs[i]);                       \
        }                                                  \
    } while (0);
#else
#define DUMPTRACE() printf("(no trace available)\n")
#endif // ifdef DUMPTRACE
