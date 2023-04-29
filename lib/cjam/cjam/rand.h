#pragma once

#include <stdlib.h>

#include "types.h"
#include "macros.h"

// xoshiro256+
struct rand { u64 s[4]; };

// rotate left
ALWAYS_INLINE u64 rand_rotl(const u64 x, int k) {
	return (x << k) | (x >> (64 - k));
}

// create new rand with specified seed
#define rand_create(_seed) ({ struct rand __r; rand_seed(&__r, (_seed)); __r; })

// splitmix64
#define rand_sm64(_u) ({                                                      \
        u64 __q = (_u + 0x9E3779B97f4A7C15);                                  \
        __q = (__q ^ (__q >> 30)) * 0xBF58476D1CE4E5B9;                       \
        __q = (__q ^ (__q >> 27)) * 0x94D049BB133111EB;                       \
        __q;                                                                  \
    })

// set seed _seed for struct rand *_prand
#define rand_seed(_prand, _seed)                                              \
    do {                                                                      \
        TYPEOF(_prand) __prand = (_prand);                                    \
        u64 __r = (_seed);                                                    \
        __prand->s[0] = (__r = rand_sm64(__r));                               \
        __prand->s[1] = (__r = rand_sm64(__r));                               \
        __prand->s[2] = (__r = rand_sm64(__r));                               \
        __prand->s[3] = (__r = rand_sm64(__r));                               \
    } while (0)

// NOTE: only works for sizeof(TYPEOF(_min, _max)) <= sizeof(u64)
// random number between _min and _max
#define rand_n(_prand, _min, _max)                                            \
    ({                                                                        \
        TYPEOF(_prand) __prand = (_prand);                                    \
        TYPEOF(_min) __min = (_min);                                          \
        TYPEOF(_max) __max = (_max);                                          \
        const u64                                                             \
            __r =                                                             \
                rand_rotl(__prand->s[0] + __prand->s[3], 23) + __prand->s[0], \
            __t = __prand->s[1] << 17;                                        \
        __prand->s[2] ^= __prand->s[0];                                       \
        __prand->s[3] ^= __prand->s[1];                                       \
        __prand->s[1] ^= __prand->s[2];                                       \
        __prand->s[0] ^= __prand->s[3];                                       \
        __prand->s[2] ^= __t;                                                 \
        __prand->s[3] = rand_rotl(__prand->s[3], 45);                         \
        ((u64) (__max - __min) == U64_MAX) ?                                  \
            ((__r))                                                           \
            : (((TYPEOF(_min)) ((__r) % (__max - __min + 1))) + __min);       \
    })

// random f64 between _min and _max
#define rand_f64(_prand, _min, _max)                                          \
    ({                                                                        \
        TYPEOF(_min) __min = (_min);                                          \
        TYPEOF(_max) __max = (_max);                                          \
        const u64 __r = rand_n((_prand), (u64) 0, U64_MAX);                   \
        const union { u64 i; f64 d; } __u =                                   \
            { .i = (((u64) 0x3FF) << 52) | (__r >> 12) };                     \
        ((__u.d - 1.0) * (__max - __min)) + __min;                            \
     })

// chance from 0.0 to 1.0
#define rand_chance(_prand, _chance)                                          \
    (rand_f64((_prand), 0.0, 1000.0) <= 1000.0 * (_chance))
