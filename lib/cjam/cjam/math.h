#pragma once

#include "log.h"
#include "types.h"
#include "macros.h"
#include "util.h"

#include <cglm/struct.h>
#include "ivec2s.h"

#include <math.h>

#define MKV2I2(_x, _y) ((ivec2s) {{ (_x), (_y) }})
#define MKV2I1(_s) ({ __typeof__(_s) __s = (_s); ((ivec2s) {{ __s, __s }}); })
#define MKV2I0() ((ivec2s) {{ 0, 0 }})
#define IVEC2S(...) (VFUNC(MKV2I, __VA_ARGS__))

#define MKV22(_x, _y) ((vec2s) {{ (_x), (_y) }})
#define MKV21(_s) ({ __typeof__(_s) __s = (_s); ((vec2s) {{ __s, __s }}); })
#define MKV20() ((vec2s) {{ 0, 0 }})
#define VEC2S(...) VFUNC(MKV2, __VA_ARGS__)

#define MKV44(_x, _y, _z, _w) ((vec4s) {{ (_x), (_y), (_z), (_w) }})
#define MKV41(_s) ({ __typeof__(_s) __s = (_s); ((vec4s) {{ __s, __s, __s, __s }}); })
#define MKV40() ((vec4s) {{ 0, 0 }})
#define VEC4S(...) VFUNC(MKV4, __VA_ARGS__)

#define PI 3.14159265359f
#define TAU (2.0f * PI)
#define PI_2 (PI / 2.0f)
#define PI_4 (PI / 4.0f)

#define deg2rad(_d) ((_d) * (PI / 180.0f))
#define rad2deg(_d) ((_d) * (180.0f / PI))

// -1, 0, 1 depending on _f < 0, _f == 0, _f > 0
#define sign(_f) ({                                                          \
    TYPEOF(_f) __f = (_f);                                                   \
    (TYPEOF(_f)) (__f < 0 ? -1 : (__f > 0 ? 1 : 0)); })

#define min(_a, _b) ({                                                       \
        TYPEOF(_a) __a = (_a), __b = (_b);                                   \
        __a < __b ? __a : __b; })

#define max(_a, _b) ({                                                       \
        TYPEOF(_a) __a = (_a), __b = (_b);                                   \
        __a > __b ? __a : __b; })

// clamp _x such that _x is in [_mi.._ma]
#define clamp(_x, _mi, _ma) (min(max(_x, _mi), _ma))

// round _n to nearest multiple of _mult
#define round_up_to_mult(_n, _mult) ({       \
        TYPEOF(_mult) __mult = (_mult);      \
        TYPEOF(_n) _m = (_n) + (__mult - 1); \
        _m - (_m % __mult);                  \
    })

// round _n to nearest multiple of _mult
#define round_up_to_multf(_n, _mult) ({      \
        TYPEOF(_mult) __mult = (_mult);      \
        TYPEOF(_n) _m = (_n) + (__mult - 1); \
        _m - fmodf(_m, __mult);              \
    })

// if abs(_x) < _e, returns _e with correct sign. otherwise returns _x
#define minabsclamp(_x, _e) ({ \
        TYPEOF(_x) __x = (_x), __e = (_e);\
        int __s = sign(__x);\
        __s = __s == 0 ? 1 : __s;\
        fabsf(__x) < __e ? (__s * __e) : (__x);\
    })

// see: https://en.wikipedia.org/wiki/Line–line_intersection

// intersect two infinite lines
#define intersect_lines(_a0, _a1, _b0, _b1) ({                               \
        TYPEOF(_a0) _l00 = (_a0), _l01 = (_a1), _l10 = (_b0), _l11 = (_b1);  \
        TYPEOF(_l00.x) _d =                                                  \
            ((_l00.x - _l01.x) * (_l10.y - _l11.y))                          \
                - ((_l00.y - _l01.y) * (_l10.x - _l11.x)),                   \
            _xl0 = cross(_l00, _l01),                                        \
            _xl1 = cross(_l10, _l11);                                        \
        (TYPEOF(_a0)) {                                                      \
            ((_xl0 * (_l10.x - _l11.x)) - ((_l00.x - _l01.x) * _xl1)) / _d,  \
            ((_xl0 * (_l10.y - _l11.y)) - ((_l00.y - _l01.y) * _xl1)) / _d   \
        };                                                                   \
    })

// intersect two line segments, returns (nan, nan) if no intersection exists
#define intersect_segs(_a0, _a1, _b0, _b1) ({                                \
        TYPEOF(_a0) _l00 = (_a0), _l01 = (_a1), _l10 = (_b0), _l11 = (_b1);  \
        TYPEOF(_l00.x)                                                       \
            _d = ((_l00.x - _l01.x) * (_l10.y - _l11.y))                     \
                    - ((_l00.y - _l01.y) * (_l10.x - _l11.x)),               \
            _t =                                                             \
                (((_l00.x - _l10.x) * (_l10.y - _l11.y))                     \
                    - ((_l00.y - _l10.y) * (_l10.x - _l11.x)))               \
                    / _d,                                                    \
            _u =                                                             \
                (((_l00.x - _l10.x) * (_l00.y - _l01.y))                     \
                    - ((_l00.y - _l10.y) * (_l00.x - _l01.x)))               \
                    / _d;                                                    \
        (isnan(_t) || isnan(_u)) ?                                           \
            ((v2) { NAN, NAN })                                              \
            : ((_t >= 0 && _t <= 1 && _u >= 0 && _u <= 1) ?                  \
                ((v2) {                                                      \
                    _l00.x + (_t * (_l01.x - _l00.x)),                       \
                    _l00.y + (_t * (_l01.y - _l00.y)) })                     \
                : ((v2) { NAN, NAN }));                                      \
    })

// true if _p is in triangle _a, _b, _c
#define point_in_triangle(_p, _a, _b, _c) ({                                 \
        TYPEOF(_p) __p = (_p), __a = (_a), __b = (_b), __c = (_c);           \
        const f32                                                            \
            _d = ((__b.y - __c.y) * (__a.x - __c.x)                          \
                  + (__c.x - __b.x) * (__a.y - __c.y)),                      \
            _x = ((__b.y - __c.y) * (__p.x - __c.x)                          \
                  + (__c.x - __b.x) * (__p.y - __c.y)) / _d,                 \
            _y = ((__c.y - __a.y) * (__p.x - __c.x)                          \
                  + (__a.x - __c.x) * (__p.y - __c.y)) / _d,                 \
            _z = 1 - _x - _y;                                                \
        (_x > 0) && (_y > 0) && (_z > 0);                                    \
    })

// -1 right, 0 on, 1 left
#define point_side(_p, _a, _b) ({                                            \
        TYPEOF(_p) __p = (_p), __a = (_a), __b = (_b);                       \
        -(((__p.x - __a.x) * (__b.y - __a.y))                                \
            - ((__p.y - __a.y) * (__b.x - __a.x)));                          \
    })

// returns fractional part of float _f
#define fract(_f) ({ TYPEOF(_f) __f = (_f); __f - ((i64) (__f)); })

// if _x is nan, returns _alt otherwise returns _x
#define ifnan(_x, _alt) ({ TYPEOF(_x) __x = (_x); isnan(__x) ? (_alt) : __x; })

// give alternative values for x if it is nan or inf
#define ifnaninf(_x, _nan, _inf) ({                                          \
        TYPEOF(_x) __x = (_x);                                               \
        isnan(__x) ? (_nan) : (isinf(__x) ? (_inf) : __x);                   \
    })

// lerp from _a -> _b by _t
#define lerp(_a, _b, _t) ({                                                  \
        TYPEOF(_t) __t = (_t);                                               \
        (TYPEOF(_a)) (((_a) * (1 - __t)) + ((_b) * __t));                    \
    })

// project point _p onto line _a -> _b
#define point_project_segment(_p, _a, _b) ({                                 \
        TYPEOF(_p) _q = (_p), _v0 = (_a), _v1 = (_b);                        \
        const f32 _l2 = powf(lengthab(_v0, _v1), 2.0f);                      \
        const f32 _t =                                                       \
            clamp(                                                           \
                dot(                                                         \
                    ((v2) { _q.x - _v0.x, _q.y - _v0.y }),                   \
                    ((v2) { _v1.x - _v0.x, _v1.y - _v0.y })) / _l2,          \
                0, 1);                                                       \
        const TYPEOF(_p) _proj = {                                           \
            _v0.x + _t * (_v1.x - _v0.x),                                    \
            _v0.y + _t * (_v1.y - _v0.y),                                    \
        };                                                                   \
        _l2 <= 0.000001f ? _v0 : _proj;                                      \
    })

// distance from _p to _a -> _b
#define point_to_segment(_p1, _a1, _b1) ({                                   \
        TYPEOF(_p1) _q1 = (_p1);                                             \
        const v2 _u1 = point_project_segment(_q1, (_a1), (_b1));             \
        lengthab(_q1, _u1);                                                  \
    })

// returns true if point _p is in box _a -> _b
#define point_in_box(_p, _a, _b) ({                                          \
        TYPEOF(_p) __p = (_p), __a = (_a), __b = (_b);                       \
        if (__a.x > __b.x) { swap(__a.x, __b.x); }                           \
        if (__a.y > __b.y) { swap(__a.y, __b.y); }                           \
        __p.x >= __a.x && __p.y >= __a.y && __p.x <= __b.x && __p.y <= __b.y;\
    })

// projects a onto b
#define vecproject(_a, _b) ({\
        TYPEOF(_a) __a = (_a), __b = (_b);\
        const f32 q = dot(__a, __b) / dot(__b, __b);\
        (v2) { q * __b.x, q * __b.y };\
    })

// intersect line segment s0 -> s1 with box defined by b0, b1
/* ALWAYS_INLINE v2 intersect_seg_box(v2 s0, v2 s1, v2 b0, v2 b1) { */
/*     if (b0.x > b1.x) { swap(b0.x, b1.x); } */
/*     if (b0.y > b1.y) { swap(b0.y, b1.y); } */

/*     v2 r; */
/*     if (!isnan((r = */
/*             intersect_segs( */
/*                 s0, s1, */
/*                 ((v2) { b0.x, b0.y }), ((v2) { b0.x, b1.y }))).x)) { */
/*         return r; */
/*     } else if ( */
/*         !isnan((r = */
/*             intersect_segs( */
/*                 s0, s1, */
/*                 ((v2) { b0.x, b1.y }), ((v2) { b1.x, b1.y }))).x)) { */
/*         return r; */
/*     } else if ( */
/*         !isnan((r = */
/*             intersect_segs( */
/*                 s0, s1, */
/*                 ((v2) { b1.x, b1.y }), ((v2) { b1.x, b0.y }))).x)) { */
/*         return r; */
/*     } else if ( */
/*         !isnan((r = */
/*             intersect_segs( */
/*                 s0, s1, */
/*                 ((v2) { b1.x, b0.y }), ((v2) { b0.x, b0.y }))).x)) { */
/*         return r; */
/*     } */

/*     return (v2) { NAN, NAN }; */
/* } */

/* ALWAYS_INLINE bool intersect_circle_circle(v2 p0, f32 r0, v2 p1, f32 r1) { */
/*     // distance between centers must be between r0 - r1, r0 + r1 */
/*     const f32 */
/*         mi = fabsf(r0 - r1), */
/*         ma = r0 + r1, */
/*         dx = p1.x - p0.x, */
/*         dy = p1.y - p0.y, */
/*         d = (dx * dx) + (dy * dy); */
/*     return mi <= d && d <= ma; */
/* } */

/* // [pr]0 are static, [pr]1 are dynamic along v */
/* ALWAYS_INLINE bool sweep_circle_circle(v2 p0, f32 r0, v2 p1, f32 r1, v2 v) { */
/*     const v2 d = */
/*         point_project_segment(p0, p1, ((v2) { p1.x + v.x, p1.y + v.y })); */

/*     return lengthab(p0, d) <= r0 + r1; */
/* } */

/* // TODO: doc */
/* ALWAYS_INLINE v2 intersct_seg_circle(v2 s0, v2 s1, v2 p, f32 r) { */
/*     // see stackoverflow.com/questions/1073336 */
/*     const v2 */
/*         d = { s1.x - s0.x, s1.y - s0.y }, */
/*         f = { s0.x - p.x, s0.y - p.y }; */

/*     const f32 */
/*         a = dot(d, d), */
/*         b = 2.0f * dot(f, d), */
/*         c = dot(f, f) - (r * r), */
/*         q = (b * b) - (4 * a * c); */

/*     if (q < 0) { */
/*         return (v2) { NAN, NAN }; */
/*     } */

/*     const f32 */
/*         r_q = sqrtf(q), */
/*         t0 = (-b - r_q) / (2 * a), */
/*         t1 = (-b + r_q) / (2 * a); */

/*     if (t0 >= 0 && t0 <= 1) { */
/*         return (v2) { s0.x + (t0 * d.x), s0.y + (t0 * d.y) }; */
/*     } else if (t1 >= 0 && t1 <= 1) { */
/*         return (v2) { s0.x + (t1 * d.x), s0.y + (t1 * d.y) }; */
/*     } */

/*     return (v2) { NAN, NAN }; */
/* } */

// noramlize angle to 0, TAU
ALWAYS_INLINE f32 wrap_angle(f32 a) {
    a = fmodf(a, TAU);
    return a < 0 ? (a + TAU) : a;
}

/* // compute inner angle formed by points p1 -> p2 -> p3 */
/* ALWAYS_INLINE f32 angle_in_points(v2 p1, v2 p2, v2 p3) { */
/*     // from stackoverflow.com/questions/3486172 and SLADE (doom level editor) */
/* 	v2 ab = { p2.x - p1.x, p2.y - p1.y }; */
/* 	v2 cb = { p2.x - p3.x, p2.y - p3.y }; */

/*     const f32 */
/*         dot = ab.x * cb.x + ab.y * cb.y, */
/*         labsq = ab.x * ab.x + ab.y * ab.y, */
/*         lcbsq = cb.x * cb.x + cb.y * cb.y; */

/* 	// square of cosine of the needed angle */
/* 	const f32 cossq = dot * dot / labsq / lcbsq; */

/* 	// apply trigonometric equality */
/* 	// cos(2a) = 2([cos(a)]^2) - 1 */
/* 	const f32 cos2 = 2 * cossq - 1; */

/*     // cos2 = cos(2a) -> a = arccos(cos2) / 2; */
/* 	f32 alpha = ((cos2 <= -1) ? PI : (cos2 >= 1) ? 0 : acosf(cos2)) / 2; */

/* 	// negative dot product -> angle is above 90 degrees. normalize. */
/* 	if (dot < 0) { */
/* 		alpha = PI - alpha; */
/*     } */

/*     // compute sign with determinant */
/* 	const f32 det = ab.x * cb.y - ab.y * cb.x; */
/* 	if (det < 0) { */
/* 		alpha = (2 * PI) - alpha; */
/*     } */

/* 	return alpha; */
/* } */

// number of 1 bits in number
#define popcount(_x) __builtin_popcountll((_x))

// number of 0 bits in (unsigned) number
#define invpopcount(_x) (_Generic((_x),                                        \
    u8:  (64 - __builtin_popcountll(0xFFFFFFFFFFFFFF00 | (u64) (_x))),         \
    u16: (64 - __builtin_popcountll(0xFFFFFFFFFFFF0000 | (u64) (_x))),         \
    u32: (64 - __builtin_popcountll(0xFFFFFFFF00000000 | (u64) (_x))),         \
    u64: (64 - __builtin_popcountll(                     (u64) (_x)))))

// (C)ount (L)eading (Z)eros
#define clz(_x) (__builtin_clz((_x)))

// (C)ount (T)railing (Z)eros
#define ctz(_x) (__builtin_ctz((_x)))
