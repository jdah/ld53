#pragma once

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "types.h"
#include "assert.h"
#include "str.h"

typedef hash_type (*f_map_hash)(void*, void*);
typedef void *(*f_map_dup)(void*, void*);
typedef void (*f_map_free)(void*, void*);
typedef int (*f_map_cmp)(void*, void*, void*);
typedef void *(*f_map_alloc)(size_t, void*, void*);

struct map_entry {
    void *key, *value;
};

struct map_internal_entry {
    struct map_entry entry;
    u16 dist;
    hash_type hash : 15;
    bool used : 1;
};

struct map {
    f_map_hash f_hash;
    f_map_dup f_keydup;
    f_map_cmp f_keycmp;
    f_map_free f_keyfree, f_valfree;

    // internal allocation functions, default to malloc/free, can be replaced
    f_map_alloc f_alloc;
    f_map_free f_free;

    usize used, capacity, prime;
    struct map_internal_entry *entries;

    void *userdata;
};

enum {
    MAP_INSERT_ENTRY_IS_NEW = 1 << 0
};

// hash functions
hash_type map_hash_id(void *p, void*);
hash_type map_hash_str(void *p, void*);

// compare functions
int map_cmp_id(void *p, void *q, void*);
int map_cmp_str(void *p, void *q, void*);

// duplication functions
void *map_dup_str(void *s, void*);

// default map_alloc_fn implemented with stdlib's "realloc()"
void *map_default_alloc(size_t n, void *p, void*);

// default map_free_fn implemented with stdlib's "free()"
void map_default_free(void *p, void*);

// internal usage only
usize _map_insert(
    struct map *self,
    hash_type hash,
    void *key,
    void *value,
    int flags,
    bool *rehash_out);

// internal usage only
usize _map_find(const struct map *self, hash_type hash, void *key);

// internal usage only
void _map_remove_at(struct map *self, usize pos);

// internal usage only
void _map_remove(struct map *self, hash_type hash, void *key);

// create new map
void map_init(
    struct map *self,
    f_map_hash f_hash,
    f_map_alloc f_alloc,
    f_map_free f_free,
    f_map_dup f_keydup,
    f_map_cmp f_keycmp,
    f_map_free f_keyfree,
    f_map_free f_valfree,
    void *userdata);

// destroy map
void map_destroy(struct map *self);

// clear map of all keys and values
void map_clear(struct map *self);

// insert (k, v) into map, replacing value if it is already present, returns
// pointer to value
#define _map_insert3(_m, _k, _v) ({                                        \
        struct map *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k), *__v = (void*)(uintptr_t)(_v); \
        usize __n =                                                        \
            _map_insert(                                                   \
                __m,                                                       \
                __m->f_hash(__k, __m->userdata),                           \
                __k,                                                       \
                __v,                                                       \
                MAP_INSERT_ENTRY_IS_NEW,                                   \
                NULL);                                                     \
        &__m->entries[__n].entry.value;                                    \
    })

// insert (k, v) into map, replacing value if it is already present, returns
// pointer T* to value
#define _map_insert4(T, _m, _k, _v) ({                                     \
        struct map *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k), *__v = (void*)(uintptr_t)(_v); \
        usize __n =                                                        \
            _map_insert(                                                   \
                __m,                                                       \
                __m->f_hash(__k, __m->userdata),                           \
                __k,                                                       \
                __v,                                                       \
                MAP_INSERT_ENTRY_IS_NEW);                                  \
        (T*) (uintptr_t) &__m->entries[__n].entry.value;                   \
    })

// insert (k, v) into map, replacing value if it is already present, returns
// pointer to value
#define map_insert(...) VFUNC(_map_insert, __VA_ARGS__)

// returns true if map contians key
#define map_contains(_m, _k) ({                                            \
        const struct map *__m = (_m);                                      \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        _map_find(__m, __m->f_hash(__k, __m->userdata), __k) != USIZE_MAX; \
    })

// returns _T *value, NULL if not present
#define _map_find3(_T, _m, _k) ({                                          \
        const struct map *__m = (_m);                                      \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        usize _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        _i == USIZE_MAX ? NULL : (_T*)(&__m->entries[_i].entry.value);     \
    })

// returns void **value, NULL if not present
#define _map_find2(_m, _k) ({                                              \
        const struct map *__m = (_m);                                      \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        usize _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        _i == USIZE_MAX ? NULL : &__m->entries[_i].entry.value;            \
    })

// map_find(map, key) or map_find(TYPE, map, key) -> ptr to value, NULL if not
// found
#define map_find(...) VFUNC(_map_find, __VA_ARGS__)

// returns _T value, crashes if not present
#define _map_get3(_T, _m, _k) ({                                           \
        struct map *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        usize _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        ASSERT(_i != USIZE_MAX, "key not found");                          \
        *(_T*)(&__m->entries[_i].entry.value);                             \
    })

// returns void *value, crashes if not present
#define _map_get2(_m, _k) ({                                               \
        struct map *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        usize _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        ASSERT(_i != USIZE_MAX, "key not found");                          \
        __m->entries[_i].entry.value;                                      \
    })

// map_get(map, key) or map_get(TYPE, map, key)
#define map_get(...) VFUNC(_map_get, __VA_ARGS__)

// remove key k from map
#define map_remove(_m, _k) ({                                              \
        struct map *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        _map_remove(__m, __m->f_hash(__k, __m->userdata), __k);            \
    })

// get next used entry from map index _i. returns _m->capacity at map end
#define map_next(_m, _i, _first) ({                                        \
        TYPEOF(_m) __m = (_m);                                             \
        usize _j = _i;                                                     \
        if (!(_first)) { _j++; }                                           \
        while (_j < __m->capacity && !__m->entries[_j].used) { _j++; }     \
        ASSERT(_j >= __m->capacity || __m->entries[_j].used);              \
        _j;                                                                \
    })

#define _map_each_impl(_KT, _VT, _m, _it, _itname)                         \
    typedef struct {                                                       \
        TYPEOF(_m) __m; usize __i; _KT key; _VT value; } _itname;         \
    for (_itname _it = {                                                   \
            .__m = (_m),                                                   \
            .__i = map_next(_it.__m, 0, true),                             \
            .key = (_KT)(_it.__i < _it.__m->capacity ?                     \
                (uintptr_t)(_it.__m->entries[_it.__i].entry.key)           \
                : 0),                                                      \
            .value = (_VT)(_it.__i < _it.__m->capacity ?                   \
                (uintptr_t)(_it.__m->entries[_it.__i].entry.value)         \
                : 0)                                                       \
         }; _it.__i < _it.__m->capacity;                                   \
         _it.__i = map_next(_it.__m, _it.__i, false),                      \
         _it.key = (_KT)(_it.__i < _it.__m->capacity ?                     \
                    (uintptr_t)(_it.__m->entries[_it.__i].entry.key)       \
                    : 0),                                                  \
         _it.value = (_VT)(_it.__i < _it.__m->capacity ?                   \
                    (uintptr_t)(_it.__m->entries[_it.__i].entry.value)     \
                    : 0))

// map_each(KEY_TYPE, VALUE_TYPE, *map, it_name)
#define map_each(_KT, _VT, _m, _it)                                        \
    _map_each_impl(                                                        \
        _KT,                                                               \
        _VT,                                                               \
        _m,                                                                \
        _it,                                                               \
        CONCAT(_mei, __COUNTER__))

// true if map is empty
#define map_empty(_m) ((_m)->used == 0)

// number of elements in map
#define map_size(_m) ((_m)->used)

#ifdef CJAM_IMPL
#include "assert.h"

#include <stdlib.h>
#include <string.h>

// planetmath.org/goodhashtableprimes
static const u32 PRIMES[] = {
    11, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};

// load factors at which rehashing happens, expressed as percentages
#define MAP_LOAD_HIGH 80
#define MAP_LOAD_LOW 10

// distance is stored as a 16-bit unsigned integer
#define MAP_MAX_DISTANCE U16_MAX

// mask for struct map_internal_entry::hash
#define MAP_STORED_HASH_MASK ((1 << 15) - 1)

hash_type map_hash_id(void *p, void*) {
    return (hash_type) (p);
}

// fnv1a hash
hash_type map_hash_str(void *p, void*) {
    char *s = p;

    hash_type h = 0;
    while (*s != '\0') {
	    h = (h ^ ((u8) (*s++))) * 1099511628211u;
    }

    return h;
}

int map_cmp_id(void *p, void *q, void*) {
    return (int) (((char*) p) - ((char*) (q)));
}

int map_cmp_str(void *p, void *q, void*) {
    return strcmp(p, q);
}

void *map_dup_str(void *s, void*) {
    return strdup(s);
}

void *map_default_alloc(size_t n, void *p, void*) {
    return CJAM_ALLOC(n, p);
}

void map_default_free(void *p, void*) {
    CJAM_FREE(p);
}

void map_init(
    struct map *map,
    f_map_hash f_hash,
    f_map_alloc f_alloc,
    f_map_free f_free,
    f_map_dup f_keydup,
    f_map_cmp f_keycmp,
    f_map_free f_keyfree,
    f_map_free f_valfree,
    void *userdata) {
    map->f_hash = f_hash;
    map->f_alloc = f_alloc ? f_alloc : map_default_alloc;
    map->f_free = f_free ? f_free : map_default_free;
    map->f_keydup = f_keydup;
    map->f_keycmp = f_keycmp;
    map->f_keyfree = f_keyfree;
    map->f_valfree = f_valfree;
    map->used = 0;
    map->capacity = 0;
    map->prime = 0;
    map->entries = NULL;
}

void map_destroy(struct map *map) {
    if (!map->entries) {
        return;
    }

    for (usize i = 0; i < map->capacity; i++)
        if (map->entries[i].used) {
            if (map->f_keyfree) {
                map->f_keyfree(map->entries[i].entry.key, map->userdata);
            }

            if (map->f_valfree) {
                map->f_valfree(map->entries[i].entry.value, map->userdata);
            }
        }

    map->f_free(map->entries, map->userdata);

    map->used = 0;
    map->capacity = 0;
    map->prime = 0;
    map->entries = NULL;
}

// returns true on rehash
static bool map_rehash(struct map *map, bool force) {
    if (!map->entries) {
        return false;
    }

    usize load = (map->used * 100) / (map->capacity);
    bool
        low = load < MAP_LOAD_LOW,
        high = load > MAP_LOAD_HIGH;

    if (!force && ((!low && !high) ||
        (low && map->prime == 0) ||
        (high && map->prime == ARRLEN(PRIMES) - 1))) {
        return false;
    }

    usize old_capacity = map->capacity;

    ASSERT(
        force ||
        (high && map->prime != ARRLEN(PRIMES) - 1) ||
        (low && map->prime != 0),
        "attempted rehash but cannot expand/shrink");

    if (low && map->prime != 0) {
        map->prime--;
    } else if (force || (high && map->prime != (ARRLEN(PRIMES) - 1))) {
        map->prime++;
    }

    ASSERT(map->prime < ARRLEN(PRIMES), "out of memory");
    map->capacity = PRIMES[map->prime];

    struct map_internal_entry *old = map->entries;
    map->entries =
        map->f_alloc(
            map->capacity * sizeof(struct map_internal_entry),
            NULL,
            map->userdata);
    memset(map->entries, 0, map->capacity * sizeof(struct map_internal_entry));

    // re-insert all entries
    for (size_t i = 0; i < old_capacity; i++) {
        if (old[i].used) {
            _map_insert(
                map,
                map->f_hash(old[i].entry.key, map->userdata),
                old[i].entry.key,
                old[i].entry.value,
                false,
                NULL);
        }
    }

    map->f_free(old, map->userdata);
    return true;
}

usize _map_insert(
    struct map *map,
    hash_type hash,
    void *key,
    void *value,
    int flags,
    bool *rehash_out) {
    if (!map->entries) {
        ASSERT(map->used == 0);
        ASSERT(map->capacity == 0);
        ASSERT(map->prime == 0);
        map->capacity = PRIMES[map->prime];
        map->entries =
            map->f_alloc(
                map->capacity * sizeof(struct map_internal_entry),
                NULL,
                map->userdata);
        memset(
            map->entries, 0, map->capacity * sizeof(struct map_internal_entry));
    }

    bool did_rehash = false;
    usize pos = hash % map->capacity, dist = 0;
    hash_type hashbits = hash & MAP_STORED_HASH_MASK;

    while (true) {
        struct map_internal_entry *entry = &map->entries[pos];
        if (!entry->used || entry->dist < dist) {
            struct map_internal_entry new_entry = {
                .entry = (struct map_entry) { .key = key, .value = value },
                .used = true,
                .dist = (u16) dist,
                .hash = hashbits
            };

            if (flags & MAP_INSERT_ENTRY_IS_NEW) {
                map->used++;
            }

            if ((flags & MAP_INSERT_ENTRY_IS_NEW) && map->f_keydup) {
                new_entry.entry.key = map->f_keydup(key, map->userdata);
            }

            if (!entry->used) {
                *entry = new_entry;
            } else {
                // distance is less than the one that we're trying to insert,
                // displace the element to reduce lookup times
                struct map_internal_entry e = *entry;
                *entry = new_entry;
                _map_insert(
                    map,
                    map->f_hash(e.entry.key, map->userdata),
                    e.entry.key,
                    e.entry.value,
                    flags & ~MAP_INSERT_ENTRY_IS_NEW,
                    &did_rehash);
            }

            break;
        } else if (
            entry->dist == dist &&
            entry->hash == hashbits &&
            !map->f_keycmp(entry->entry.key, key, map->userdata)) {
            // entry already exists, replace value
            if (map->f_valfree) {
                map->f_valfree(entry->entry.value, map->userdata);
            }

            entry->entry.value = value;
            break;
        }

        // rehash if we have a greater distance than storage allows
        dist++;
        if (dist >= MAP_MAX_DISTANCE) {
            did_rehash = true;
            if (rehash_out) { *rehash_out = true; }
            map_rehash(map, true);
            return _map_insert(map, hash, key, value, flags, NULL);
        }

        // check next location
        pos = (pos + 1) % map->capacity;
    }

    // rehash if necessary
    if (map_rehash(map, false) || did_rehash) {
        // recompute pos as it may have moved on rehash
        if (rehash_out) { *rehash_out = true; }
        pos = _map_find(map, hash, key);
    }

    return pos;
}

usize _map_find(const struct map *map, hash_type hash, void *key) {
    if (!map->entries) { return USIZE_MAX; }

    usize pos = hash % map->capacity, res = USIZE_MAX;
    hash_type hashbits = hash & MAP_STORED_HASH_MASK;

    while (true) {
        if (!map->entries[pos].used) {
            break;
        }

        if (map->entries[pos].hash == hashbits
            && !map->f_keycmp(map->entries[pos].entry.key, key, map->userdata)) {
            res = pos;
            break;
        }

        pos = (pos + 1) % map->capacity;
    }

    return res;
}

void _map_remove_at(struct map *map, usize pos) {
    ASSERT(map->entries);

    usize next_pos = 0;
    ASSERT(pos < map->capacity, "could not remove key at pos %" PRIusize, pos);

    struct map_internal_entry *entry = &map->entries[pos];
    if (entry->entry.key && map->f_keyfree) {
        map->f_keyfree(entry->entry.key, map->userdata);
    }

    if (entry->entry.value && map->f_valfree) {
        map->f_valfree(entry->entry.value, map->userdata);
    }

    map->used--;

    // mark empty
    entry->used = false;

    // shift further entries back by one
    while (true) {
        next_pos = (pos + 1) % map->capacity;

        // stop when unused entry is found or dist is 0
        if (!map->entries[next_pos].used
            || map->entries[next_pos].dist == 0) {
            break;
        }

        // copy next pos into current
        map->entries[pos] = map->entries[next_pos];
        map->entries[pos].dist--;

        // mark next entry as empty
        map->entries[next_pos].used = false;

        pos = next_pos;
    }

    map_rehash(map, false);
}

void _map_remove(struct map *map, hash_type hash, void *key) {
    const usize pos = _map_find(map, hash, key);
    ASSERT(pos != USIZE_MAX, "could not find key with hash %" PRIusize, hash);
    _map_remove_at(map, pos);
    ASSERT(!map_find(map, key));
}

void map_clear(struct map *map) {
    map_destroy(map);
}
#endif // ifdef CJAM_IMPL
