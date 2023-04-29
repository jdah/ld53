#pragma once

#include "types.h"
#include "macros.h"
#include "util.h"
#include "assert.h"

// create an enum with auto-generated *_to_str(...) function,
// *_COUNT: last element + 1
// *_DISTINCT: number of distinct elements
// *_MASK: all elements OR'd with each other (useful for flag sets)
//
// usage:
// ENUM_<NAME>(_F, ...)                     <backslash>
//   _F(SOMETHING, <value>, __VA_ARGS__)    <backslash>
//   _F(ANOTHER, <value>, __VA_ARGS__)      <backslash>
//
// ENUM_MAKE(type_name, PREFIX, ENUM_<NAME>)
#define _ENUM_MAKE4(_U, _V, _N) _N##_##_U,
#define _ENUM_MAKE3(_U, _V, _N) | _V
#define _ENUM_MAKE2(...) + 1
#define _ENUM_MAKE1(_U, ...) #_U,
#define _ENUM_MAKE0(_U, _V, _N) _N##_##_U = (_V),
#define ENUM_MAKE(_T, _N, _E)                                                  \
    enum _T {                                                                  \
        _E(_ENUM_MAKE0, _N)                                                    \
        _N##_COUNT,                                                            \
        _N##_DISTINCT = 0 _E(_ENUM_MAKE2),                                     \
        _N##_MASK     = 0 _E(_ENUM_MAKE3),                                     \
    };                                                                         \
    const char **_T##_names();                                                 \
    bool _T##_is_valid(int value);                                             \
    const char *_T##_to_str(enum _T x);                                        \
    enum _T _T##_nth(int n);                                                   \
    int _T##_nth_value(int n);

// defines some ENUM_MAKE helper functions
//
// *_names:
// returns table of names in the order that they appear in the enum declaration
//
// *_is_valid:
// returns true if value is a valid enum member
//
// *_to_str:
// construct a hash table of string elements to back *_to_str functions
// hashtable is used over simple array so that sparse enums don't take up extra
// space
//
// *_nth:
// gets nth distinct value of enum
//
// *_nth_value:
// gets nth distinct value of enum as int
#define ENUM_DEFN_FUNCTIONS(_T, _N, _E)                                        \
    static enum _T _T##_VALS[_N##_DISTINCT] = { _E(_ENUM_MAKE4, _N) };         \
    static const char *_T##_NAMES[_N##_DISTINCT] = { _E(_ENUM_MAKE1, _N) };    \
    const char **_T##_names() {                                                \
        return _T##_NAMES;                                                     \
    }                                                                          \
    bool _T##_is_valid(int value) {                                            \
        for (int i = 0; i < _N##_DISTINCT; i++) {                              \
            if (((int) _T##_VALS[i]) == value) { return true; }                \
        }                                                                      \
        return false;                                                          \
    }                                                                          \
    const char *_T##_to_str(enum _T x) {                                       \
        static bool _T##_LOOKUP_INIT;                                          \
        static struct { const char *s; enum _T e; } _T##_LOOKUP[_N##_DISTINCT];\
        if (!_T##_LOOKUP_INIT) {                                               \
            _T##_LOOKUP_INIT = true;                                           \
            for (int i = 0; i < _N##_DISTINCT; i++) {                          \
                enum _T v = _T##_VALS[i];                                      \
                u64 pos = hashbytes((u8*) &v, sizeof(v)) % _N##_DISTINCT;      \
                while (_T##_LOOKUP[pos].s) { pos = (pos + 1) % _N##_DISTINCT; }\
                _T##_LOOKUP[pos].s = _T##_NAMES[i];                            \
                _T##_LOOKUP[pos].e = v;                                        \
            }                                                                  \
        }                                                                      \
        u64 pos = hashbytes((u8*) &x, sizeof(x)) % _N##_DISTINCT;              \
        while (_T##_LOOKUP[pos].e != x) { pos = (pos + 1) % _N##_DISTINCT; }   \
        return _T##_LOOKUP[pos].s;                                             \
    }                                                                          \
    enum _T _T##_nth(int n) {                                                  \
        ASSERT(n >= 0 && n <= _N##_DISTINCT);                                  \
        return _T##_VALS[n];                                                   \
    }                                                                          \
    int _T##_nth_value(int n) {                                                \
        ASSERT(n >= 0 && n <= _N##_DISTINCT);                                  \
        return (int) _T##_VALS[n];                                             \
    }
