#pragma once

#include <stdlib.h>

#include "config.h"
#include "assert.h"
#include "macros.h"
#include "types.h"
#include "math.h"

// usage: like FILE*, always use as BITMAP*
typedef u8 BITMAP;

// _sz number of bits -> bytes
#define BITMAP_SIZE_TO_BYTES(_sz) (((_sz) + 7) / 8)

// excess bits on bitmap of size _sz, i.e. size 11 = 2 bytes -> 5 extra bits
#define BITMAP_EXCESS_BITS(_sz) (BITMAP_SIZE_TO_BYTES((_sz)) * 8 - (_sz))

// declare a bitmap:
// BITMAP_DECL(bits, 32); -> "u8 bits[...]"
#define BITMAP_DECL(_name, _size) u8 _name[BITMAP_SIZE_TO_BYTES(_size)]

ALWAYS_INLINE BITMAP *bitmap_alloc(int sz) {
    return CJAM_ALLOC(BITMAP_SIZE_TO_BYTES(sz), NULL);
}

ALWAYS_INLINE BITMAP *bitmap_calloc(int sz) {
  BITMAP *res = CJAM_ALLOC(BITMAP_SIZE_TO_BYTES(sz), NULL);
  memset(res, 0, BITMAP_SIZE_TO_BYTES(sz));
  return res;
}

ALWAYS_INLINE BITMAP *bitmap_realloc(BITMAP *b, int sz) {
    return CJAM_ALLOC(BITMAP_SIZE_TO_BYTES(sz), b);
}

ALWAYS_INLINE void bitmap_free(BITMAP *b) {
    CJAM_FREE(b);
}

ALWAYS_INLINE void bitmap_set(BITMAP *b, int n) {
    ((u8*) b)[n / 8] |= (1 << (n % 8));
}

ALWAYS_INLINE bool bitmap_get(const BITMAP *b, int n) {
    return !!(((u8*)b)[n / 8] & (1 << (n % 8)));
}

ALWAYS_INLINE void bitmap_clr(BITMAP *b, int n) {
    ((u8*) b)[n / 8] &= ~(1 << (n % 8));
}

ALWAYS_INLINE void bitmap_put(BITMAP *b, int n, bool val) {
    if (val) { bitmap_set(b, n); } else { bitmap_clr(b, n); }
}

ALWAYS_INLINE void bitmap_fill(BITMAP *b, int size, bool val) {
    const u8 v = val ? 0xFF : 0x00;
    for (int i = 0; i < BITMAP_SIZE_TO_BYTES(size); i++) {
        b[i] = v;
    }
}

// count number of bits with value
ALWAYS_INLINE int bitmap_count(const BITMAP *b, int size, bool val) {
    if (size == 0) { return 0; }

    const u8 *p = b, *end = b + BITMAP_SIZE_TO_BYTES(size);
    int n = 0;

#define COUNT_FOR_T(_T)                                             \
    while ((end - p) >= (int) sizeof(_T)) {                         \
        const _T t = *((_T*) p);                                    \
        n += val ? popcount(t) : invpopcount(t);                    \
        p += sizeof(_T);                                            \
    }

    COUNT_FOR_T(u64)
    COUNT_FOR_T(u32)
    COUNT_FOR_T(u16)
    COUNT_FOR_T(u8)

#undef COUNT_FOR_T

    // don't count extra bits on the last byte
    if (size % 8 != 0) {
        // mask of extra bits on end of bitmap
        // fx. size = 11, size_bytes = 2, total size bits = 16
        //     data     | garbage
        // 00000000 000 | 01010
        // exmask = 000 | 11111
        // extra  = 000 | 01010
        const u8
            exmask = ~((1 << (8 - BITMAP_EXCESS_BITS(size))) - 1),
            extra = (*(end - 1) & exmask) | (val ? 0 : ~exmask);

        n -= val ? popcount(extra) : invpopcount(extra);
    }

    return n;
}

// returns index of lowest bit with value or INT_MAX if there is no such bit
ALWAYS_INLINE int bitmap_find(
    const BITMAP *b,
    int size,
    int start,
    bool val) {
    if (size == 0) { return INT_MAX; }
    ASSERT(start < size);

    const u8 *p = b + (start / 8), *end = b + BITMAP_SIZE_TO_BYTES(size);

    // search start value - offset according to start if not byte aligned
    int from = start % 8;

    while (p != end) {
        if ((val ? popcount(*p) : invpopcount(*p)) != 0) {
            const int to =
                p == (end - 1) ? (8 - BITMAP_EXCESS_BITS(size)) : 8;

            for (int i = from; i < to; i++) {
                if (val == !!(*p & (1 << i))) {
                    return (8 * ((int) (p - b))) + i;
                }
            }
        }

        from = 0;
        p++;
    }

    return INT_MAX;
}
