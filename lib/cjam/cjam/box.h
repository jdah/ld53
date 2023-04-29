#pragma once

#include <cjam/macros.h>
#include <cjam/types.h>
#include <cjam/math.h>

#define BOXI2F(_a)                                                      \
    ({ TYPEOF(_a) __a = (_a);                                           \
       (boxf) { AS_VEC2S(__a.min), vec2s_add(AS_VEC2S(__a.max), VEC2S(1)) }; })

#define BOXF2I(_a)                                                      \
    ({ TYPEOF(_a) __a = (_a);                                           \
       (box) { AS_IVEC2S(__a.min), AS_IVEC2S(__a.max) }; })

// 2D float box
typedef struct {
    vec2s min, max;
} boxf;

// 2D int box
typedef struct {
    ivec2s min, max;
} box;

#define BOXF_MM(_mi, _ma) ((boxf) { (_mi), (_ma) })
#define BOX_MM(_mi, _ma) ((box) { (_mi), (_ma) })

#define BOXF_M(_ma) ((boxf) { VEC2S(0), (_ma) })
#define BOX_M(_ma) ((box) { IVEC2S(0), (_ma) })

#define BOXF_PS(_pos, _size) ({                                      \
        const vec2s __pos = (_pos);                                     \
        ((boxf) { __pos, vec2s_add(__pos, (_size)));                    \
    })

#define BOX_PS(_pos, _size) ({                                       \
        const ivec2s __pos = (_pos);                                    \
        ((box) { __pos, glms_ivec2_add(__pos, glms_ivec2_sub((_size), IVEC2S(1))) }); \
    })

#define BOXF_CH(_center, _half) ({                                   \
        const vec2s __c = (_center), __h = (_half);                     \
        ((boxf) { vec2s_sub(__c, __h), vec2s_add(__c, __h)});              \
    })

#define BOX_CH(_center, _half) ({                                    \
        const vec2s __c = (_center), __h = (_half);                     \
        ((box) { glms_ivec2_sub(__c, __h), glms_ivec2_add(__c, __h)});             \
    })

// returns true if box contains p
#define _box_contains_impl(_name, _T, _t, _V, _Q, _U, _s)                   \
    ALWAYS_INLINE bool _name(_T box, _V p) {                            \
        return p.x >= box.min.x                                         \
            && p.x <= box.max.x                                         \
            && p.y >= box.min.y                                         \
            && p.y <= box.max.y;                                        \
    }

// returns true if a and b collide
#define _box_collides_impl(_name, _T, _t, _V, _Q, _U, _s)                   \
    ALWAYS_INLINE bool _name(_T a, _T b) {                              \
        UNROLL(2)                                                       \
        for (usize i = 0; i < 2; i++) {                                 \
            if (a.min.raw[i] >= b.max.raw[i]                            \
                || a.max.raw[i] <= b.min.raw[i]) {                      \
                return false;                                           \
            }                                                           \
        }                                                               \
                                                                        \
        return true;                                                    \
    }

// returns overlap of a and b
#define _box_intersect_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _T b) {                                \
        _T r;                                                           \
                                                                        \
        UNROLL(2)                                                       \
        for (usize i = 0; i < 2; i++) {                                 \
            r.min.raw[i] =                                              \
                a.min.raw[i] < b.min.raw[i] ?                           \
                    b.min.raw[i] : a.min.raw[i];                        \
            r.max.raw[i] =                                              \
                a.max.raw[i] > b.max.raw[i] ?                           \
                    b.max.raw[i] : a.max.raw[i];                        \
        }                                                               \
                                                                        \
        return r;                                                       \
    }

// returns center of box
#define _box_center_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    ALWAYS_INLINE _V _name(_T a) {                                      \
        return glms_##_Q##_add(glms_##_Q##_divs(glms_##_Q##_sub(a.max, a.min), 2), a.min);   \
    }

// translates box by v
#define _box_translate_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        return (_T) { glms_##_Q##_add(a.min, v), glms_##_Q##_add(a.max, v) };         \
    }

// scales box by v, keeping center constant
#define _box_scale_center_impl(_name, _T, _t, _V, _Q, _U, _s)               \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        _V                                                              \
            c = _t##_center(a),                                         \
            d = glms_##_Q##_sub(a.max, a.min),                                 \
            h = glms_##_Q##_divs(d, 2),                                        \
            e = glms_##_Q##_mul(h, v);                                         \
        return (_T) { glms_##_Q##_sub(c, e), glms_##_Q##_add(c, e) };                 \
    }

// scales box by v, keeping min constant
#define _box_scale_min_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        _V d = glms_##_Q##_sub(a.max, a.min);                                  \
        return (_T) { a.min, glms_##_Q##_add(a.min, glms_##_Q##_mul(d, v)) };         \
    }


// scales box by v, both min and max
#define _box_scale_impl(_name, _T, _t, _V, _Q, _U, _s)                      \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        return (_T) { glms_##_Q##_mul(a.min, v), glms_##_Q##_mul(a.max, v) };         \
    }

// get points of box
#define _box_points_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    ALWAYS_INLINE void _name(_T a, _V ps[4]) {                          \
        ps[0] = _U(a.min.x, a.min.y);                                   \
        ps[1] = _U(a.min.x, a.max.y);                                   \
        ps[2] = _U(a.max.x, a.max.y);                                   \
        ps[3] = _U(a.max.x, a.min.y);                                   \
    }

// get "half" (half size) of box
#define _box_half_impl(_name, _T, _t, _V, _Q, _U, _s)                       \
    ALWAYS_INLINE _V _name(_T a) {                                      \
        return glms_##_Q##_sub(_t##_center(a), a.min);                         \
    }

// center box on new point
#define _box_center_on_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _V c) {                                \
        _V half = _t##_half(a);                                         \
        return (_T) { glms_##_Q##_sub(c, half), glms_##_Q##_add(c, half)};            \
    }

// center box in another box
#define _box_center_in_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _T b) {                                \
        return _t##_center_on(a, _t##_center(b));                       \
    }

#define _box_define_functions(_prefix, _T, _t, _V, _Q, _U, _S)              \
    _box_contains_impl(_prefix##_contains, _T, _t, _V, _Q, _U, _S)          \
    _box_collides_impl(_prefix##_collides, _T, _t, _V, _Q, _U, _S)          \
    _box_intersect_impl(_prefix##_intersect, _T, _t, _V, _Q, _U, _S)        \
    _box_center_impl(_prefix##_center, _T, _t, _V, _Q, _U, _S)              \
    _box_translate_impl(_prefix##_translate, _T, _t, _V, _Q, _U, _S)        \
    _box_scale_center_impl(_prefix##_scale_center, _T, _t, _V, _Q, _U, _S)  \
    _box_scale_min_impl(_prefix##_scale_min, _T, _t, _V, _Q, _U, _S)        \
    _box_scale_impl(_prefix##_scale, _T, _t, _V, _Q, _U, _S)                \
    _box_half_impl(_prefix##_half, _T, _t, _V, _Q, _U, _s)                  \
    _box_center_on_impl(_prefix##_center_on, _T, _t, _V, _Q, _U, _s)        \
    _box_center_in_impl(_prefix##_center_in, _T, _t, _V, _Q, _U, _s)        \
    _box_points_impl(_prefix##_points, _T, _t, _V, _Q, _U, _S)

/* // returns size of box */
/* ALWAYS_INLINE vec2s boxf_size(boxf a) { */
/*     return vec2s_sub(a.max, a.min); */
/* } */

// returns size of box
ALWAYS_INLINE ivec2s box_size(box a) {
    return glms_ivec2_add(glms_ivec2_sub(a.max, a.min), IVEC2S(1));
}

_box_define_functions(boxf, boxf, boxf, vec2s, vec2, VEC2S, f32)
_box_define_functions(box, box, box, ivec2s, ivec2, IVEC2S, int)
