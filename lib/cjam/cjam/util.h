#pragma once

#include "types.h"
#include "macros.h"

// swap _a and _b
#define swap(_a, _b) ({ TYPEOF(_a) _x = (_a); _a = (_b); _b = (_x); })

ALWAYS_INLINE void memset32(void *dst, u32 val, usize n) {
    u32 *dst32 = dst;
    while (n--) { *dst32++ = val; }
}

ALWAYS_INLINE void memset16(void *dst, u16 val, usize n) {
    u16 *dst16 = dst;
    while (n--) { *dst16++ = val; }
}

ALWAYS_INLINE void memsetf64(void *dst, f64 val, usize n) {
    f64 *dst64 = dst;
    while (n--) { *dst64++ = val; }
}

ALWAYS_INLINE void memsetf32(void *dst, f32 val, usize n) {
    f32 *dst32 = dst;
    while (n--) { *dst32++ = val; }
}

ALWAYS_INLINE u64 hashbytes(u8 *bytes, usize n) {
    u32 seed = 0x12345;
    for (usize i = 0; i < n; i++) {
        seed ^= bytes[i] + 0x9E3779B9u + (seed << 6) + (seed >> 2);
    }
    return seed;
}
