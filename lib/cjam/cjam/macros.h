#pragma once

#include <assert.h>

// PLATFORM_POSIX for systems with POSIX headers
#if defined (__unix__)                             \
    || (defined (__APPLE__) && defined (__MACH__)) \
    || defined(__CYGWIN__)                         \
    || defined(__MINGW32__)                        \
    || defined(__EMSCRIPTEN__)
#define PLATFORM_POSIX
#endif

// platform identification
#if defined(__EMSCRIPTEN__)
#define PLATFORM_EMSCRIPTEN
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_OSX
#elif defined(__linux__)
#define PLATFORM_LINUX
#elif defined(_WIN32)
#define PLATFORM_WINDOWS
#endif

// convert preprocessor value x to string
#define _STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) _STRINGIFY_IMPL(x)

// cross-platform pragma
#define PRAGMA(x) _Pragma(#x)

// concat two preprocessor variables
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

// loop unroll
#if defined(__clang__)
#define UNROLL(n) PRAGMA(clang loop unroll_count(n))
#elif defined(__GNU__)
#define UNROLL(n) PRAGMA(GCC unroll n)
#else
#define UNROLL(n) PRAGMA(unroll)
#endif

// typeof(expr)
#define TYPEOF(_t) __typeof__((_t))

// type "const _t" -> "_t"
#define UNCONST(_t) __typeof__(({ _t __x; __auto_type __y = __x; __y; }))

// value is _t if _v is null, otherwith _v
#define IFNULL(_v, _t) ({ TYPEOF(_v) __v = (_v); __v ? __v : (_t); })

#define UNUSED __attribute__((unused))
#define PACKED __attribute__((packed))

// force inlining
#define ALWAYS_INLINE static inline __attribute__((always_inline)) UNUSED

// usage: STATIC_ASSERT(<expr>, <message>)
#if __STDC_VERSION__ >= 202311L
#define STATIC_ASSERT static_assert
#else
#define STATIC_ASSERT _Static_assert
#endif

// generates a warning if not the same type
#define CHECK_TYPE(_T, _a) ((void) (((TYPEOF(_a)*) (NULL)) == (_T*)(NULL)))

// generates error if field does not exist
#define CHECK_FIELD(_T, _f) ((void) ((_T) {})._f)

// see stackoverflow.com/questions/11761703
// get number of arguments with _NARG_
#define NARG(...) _NARG_I_(__VA_ARGS__,_RSEQ_N())
#define _NARG_I_(...) _ARG_N(__VA_ARGS__)
#define _ARG_N(                               \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,  \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define _RSEQ_N()                             \
     63,62,61,60,                             \
     59,58,57,56,55,54,53,52,51,50,           \
     49,48,47,46,45,44,43,42,41,40,           \
     39,38,37,36,35,34,33,32,31,30,           \
     29,28,27,26,25,24,23,22,21,20,           \
     19,18,17,16,15,14,13,12,11,10,           \
     9,8,7,6,5,4,3,2,1,0

// general definition for any function name
#define _VFUNC(name, n) CONCAT(name, n)
#define VFUNC(func, ...) _VFUNC(func, NARG(__VA_ARGS__))(__VA_ARGS__)

#define _NTH_ARG0(A0, ...) A0
#define _NTH_ARG1(A0, A1, ...) A1
#define _NTH_ARG2(A0, A1, A2, ...) A2
#define _NTH_ARG3(A0, A1, A2, A3, ...) A3
#define _NTH_ARG4(A0, A1, A2, A3, A4, ...) A4
#define _NTH_ARG5(A0, A1, A2, A3, A4, A5, ...) A5
#define _NTH_ARG6(A0, A1, A2, A3, A4, A5, A6, ...) A6
#define _NTH_ARG7(A0, A1, A2, A3, A4, A5, A6, A7, ...) A7
#define _NTH_ARG8(A0, A1, A2, A3, A4, A5, A6, A7, A8, ...) A8

// get NTH_ARG
#define _NTH_ARG(N) CONCAT(_NTH_ARG, N)
#define NTH_ARG(N, ...) _NTH_ARG(NARG(__VA_ARGS__))(__VA_ARGS__)
