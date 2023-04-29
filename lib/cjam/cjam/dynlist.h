#pragma once

#include "config.h"
#include "types.h"
#include "macros.h"

// usage: DYNLIST(struct foo) foolist;
#define DYNLIST(_T) TYPEOF((_T*)(NULL))

// minimal initial capacity
#define DYNLIST_INIT_CAP 4

// minimal capacity
#define DYNLIST_MIN_CAP 1

// internal use only
// header of dynlist
struct dynlist_header {
    int size, capacity;
    void *userdata;

    // TODO: hack!
#ifdef EMSCRIPTEN
    void *padding;
#endif
};

STATIC_ASSERT(sizeof(struct dynlist_header) % 16 == 0, "fix");

// internal use only
// allocates a dynlist
void _dynlist_alloc_impl(void **plist, int tsize);

// internal use only
// frees a dynlist
void _dynlist_free_impl(void **plist, int tsize);

// realloc flags
enum {
    _DYNLIST_RF_NONE = 0,
    _DYNLIST_RF_NO_CONTRACT = 1 << 0
};

// internal use only
// resizes internal list buffer to accomodate (at least) newcap
void _dynlist_realloc_impl(void **plist, int tsize, int newcap, int flags);

// internal use only
// inserts into list at index, returns pointer to allocated space
void *_dynlist_insert_impl(void **plist, int tsize, int index);

// internal use only
// removes from list at index
void _dynlist_remove_impl(void **plist, int tsize, int index);

// internal use only
// changes list size (not capacity) to n elements, ensuring space if necessary
void _dynlist_resize_impl(void **plist, int tsize, int n);

// internal use only
// copies a dynlist
void *_dynlist_copy_impl(void **plist, int tsize);

// internal use only
// copy psrc to pdst
void _dynlist_copy_from_impl(void **pdst, void **psrc, int tsize);

// pointer to header of dynlist, NULL if NULL
#define dynlist_header(_q) ({                                                \
        TYPEOF(_q) __q = (_q);                                               \
        (struct dynlist_header*) (                                           \
            __q ? (((void*) __q) - sizeof(struct dynlist_header)) : NULL);   \
    })

// returns POINTER TO header userdata pointer, NULL if it does not exist or
// list is NULL
#define dynlist_userdata_ptr(_d) ({                                          \
        struct dynlist_header *_h = dynlist_header((_d));                    \
        _h ? &_h->userdata : NULL;                                           \
    })

// allocate dynlist
#define dynlist_alloc(_d)                                                    \
    _dynlist_alloc_impl((void**) &(_d), sizeof(*(_d)) /* NOLINT */)

// free dynlist
#define dynlist_free(_d) ({                                                  \
        TYPEOF(_d) *_pd = &(_d);                                             \
        _dynlist_free_impl((void**) _pd, sizeof(*(_d)) /* NOLINT */);        \
        *_pd = NULL;                                                         \
    })

// returns size of dynlist
#define dynlist_size(_d)                                                     \
    ({ TYPEOF(_d) __d = (_d); __d ? dynlist_header(__d)->size : 0; })

// returns size of dynlist in bytes
#define dynlist_size_bytes(_d) ({                                            \
        TYPEOF(_d) __d = (_d);                                               \
        __d ? dynlist_header(__d)->size * sizeof(*(_d) /* NOLINT */): 0;     \
    })

// returns internal allocated capacity of dynlist
#define dynlist_capacity(_d)                                                 \
    ({ TYPEOF(_d) __d = (_d); _d ? dynlist_header(__d)->capacity : 0; })

// ensure capacity of at least _n elements
#define dynlist_ensure(_d, _n)                                               \
    _dynlist_realloc_impl(                                                   \
        (void**) &(_d), sizeof(*(_d)) /* NOLINT */, (_n), _DYNLIST_RF_NONE)

// insert new element at _i, returns pointer to new element
#define dynlist_insert(_d, _i)                                               \
    (TYPEOF(&(_d)[0])) _dynlist_insert_impl(                                 \
        (void**) &(_d), sizeof(*(_d)) /* NOLINT */, (_i))

// prepend, returns pointer to new space
#define dynlist_prepend(_d)                                                  \
    (TYPEOF(&(_d)[0])) _dynlist_insert_impl(                                 \
        (void**) &(_d), sizeof(*(_d)) /* NOLINT */, 0)

// append, returns pointer to new space
#define dynlist_append(_d) ({                                                \
        TYPEOF(_d) *_pd = &(_d);                                             \
        struct dynlist_header *h = dynlist_header(*_pd);                     \
        (TYPEOF(&(_d)[0])) _dynlist_insert_impl(                             \
            (void**) _pd, sizeof((_d)[0]) /* NOLINT */, h ? h->size : 0);    \
    })

// remove from dynlist at index, returns element
#define dynlist_remove(_d, _i) ({                                            \
        TYPEOF(_d) *_pd = &(_d);                                             \
        TYPEOF(_i) __i = (_i);                                               \
        TYPEOF((_d)[0]) _t = (*_pd)[__i];                                    \
        _dynlist_remove_impl((void**) _pd, sizeof(*(_d)) /* NOLINT */, __i); \
        _t;                                                                  \
    })

// pop from dynlist, returns element
#define dynlist_pop(_d) ({                                                   \
        TYPEOF(_d) *_pd = &(_d);                                             \
        struct dynlist_header *h = dynlist_header(*_pd);                     \
        TYPEOF(*(_d)) _t = (*_pd)[h->size - 1];                              \
        _dynlist_remove_impl(                                                \
            (void**) _pd, sizeof(*(_d)) /* NOLINT */, h->size - 1);          \
        _t;                                                                  \
    })

// push, returns pointer to new space
#define dynlist_push dynlist_append

// sets dynlist size - will NOT contract, but may expand or allocate if needed
// to ensure at least _n elements are available
#define dynlist_resize(_d, _n)                                               \
    _dynlist_resize_impl((void**) &(_d), sizeof(*(_d)) /* NOLINT */, _n)

#define _dynlist_each_impl(_d, _it, _start, _end, _pname, _itname)           \
    typedef struct {                                                         \
        int i, _dl_end;                                                      \
        UNCONST(TYPEOF(&(_d)[0])) el; } _itname;                             \
    TYPEOF(&(_d)) _pname = &(_d);                                            \
    for (_itname _it = ({                                                    \
            TYPEOF(_start) __start = (_start);                               \
            (_itname) {                                                      \
                .i = __start,                                                \
                ._dl_end = (_end),                                           \
                .el = (*_pname) ? &(*_pname)[__start] : NULL                 \
            };                                                               \
         });                                                                 \
         _it.i < dynlist_size(*_pname)                                       \
            && _it.i < _it._dl_end;                                          \
         _it.i++,                                                            \
         (_it.el = (*_pname) ? &(*_pname)[_it.i] : NULL))

#define _dynlist_each4(_l, _it, _start, _end)                                \
    _dynlist_each_impl(                                                      \
        _l,                                                                  \
        _it,                                                                 \
        _start,                                                              \
        _end,                                                                \
        CONCAT(_dlp, __COUNTER__),                                           \
        CONCAT(_dli, __COUNTER__))

#define _dynlist_each3(_l, _it, _start)                                      \
    _dynlist_each_impl(                                                      \
        _l,                                                                  \
        _it,                                                                 \
        _start,                                                              \
        INT_MAX,                                                             \
        CONCAT(_dlp, __COUNTER__),                                           \
        CONCAT(_dli, __COUNTER__))

#define _dynlist_each2(_l, _it)                                              \
    _dynlist_each_impl(                                                      \
        _l,                                                                  \
        _it,                                                                 \
        0,                                                                   \
        INT_MAX,                                                             \
        CONCAT(_dlp, __COUNTER__),                                           \
        CONCAT(_dli, __COUNTER__))

// usage: dynlist_each(<list>, <it>, [start?], [end?]) { ... }
// NOTE: list expansion is supported under iteration, but to remove the
// currently iterated element, dynlist_remove_it *must* be used.
#define dynlist_each(...) VFUNC(_dynlist_each, __VA_ARGS__)

// remove current iterated element under iteration
#define dynlist_remove_it(_d, _it) do {                                      \
        TYPEOF(&(_d)) __pd = &(_d);                                          \
        dynlist_remove(*__pd, (_it).i);                                      \
        (_it).i--;                                                           \
    } while(0)

// create copy of a dynlist
#define dynlist_copy(_d)                                                     \
    ((TYPEOF(_d))                                                            \
        _dynlist_copy_impl((void**) &(_d), sizeof(*(_d)) /* NOLINT */))

// copy dynlist into another dynlist, returns _dst
#define dynlist_copy_from(_dst, _src) ({                                     \
        CHECK_TYPE(TYPEOF(*(_dst)), *(_src));                                \
        _dynlist_copy_from_impl(                                             \
            (void**) &(_dst),                                                \
            (void**) &(_src),                                                \
            sizeof(*(_dst)) /* NOLINT */);                                   \
    })

#ifdef CJAM_IMPL
#include "math.h"
#include "assert.h"

void _dynlist_alloc_impl(void **plist, int tsize) {
    struct dynlist_header *h =
        CJAM_ALLOC(
            sizeof(struct dynlist_header) + (tsize * DYNLIST_INIT_CAP),
            NULL);
    *h = (struct dynlist_header) {
        .size = 0,
        .capacity = DYNLIST_INIT_CAP,
        .userdata = NULL,
    };
    *plist = h + 1;
    ASSERT((uintptr_t) (*plist) % 16 == 0);
}

void _dynlist_free_impl(void **plist, int tsize) {
    if (*plist) {
        CJAM_FREE(*plist - sizeof(struct dynlist_header));
        *plist = NULL;
    }
}

void _dynlist_realloc_impl(void **plist, int tsize, int newcap, int flags) {
    ASSERT(newcap >= 0);

    if (newcap == 0) {
        _dynlist_free_impl(plist, tsize);
        return;
    }

    if (!*plist && newcap != 0) {
        _dynlist_alloc_impl(plist, tsize);
    }

    struct dynlist_header *h = dynlist_header(*plist);

    // never allow 0 capacity from this point
    int capacity = max(h->capacity, 1);

    // TODO: loops bad
    while (newcap > capacity) {
        capacity *= 2;
    }

    if (!(flags & _DYNLIST_RF_NO_CONTRACT)) {
        while (newcap <= (capacity / 2)) {
            capacity /= 2;
        }
    }

    if (h->capacity != capacity) {
        h->capacity = capacity;
        h = CJAM_ALLOC(sizeof(*h) + (h->capacity * tsize), h);
        *plist = h + 1;
    }
}

void *_dynlist_insert_impl(void **plist, int tsize, int index) {
    if (!*plist) {
        _dynlist_alloc_impl(plist, tsize);
    }

    _dynlist_realloc_impl(
        plist,
        tsize,
        dynlist_header(*plist)->size + 1,
        _DYNLIST_RF_NO_CONTRACT);

    struct dynlist_header *h = dynlist_header(*plist);
    void *data = h + 1;

    if (index != h->size - 1) {
        memmove(
            data + (tsize * (index + 1)),
            data + (tsize * (index + 0)),
            (h->size - index) * tsize);
    }

    h->size++;
    return data + (tsize * index);
}

void _dynlist_remove_impl(void **plist, int tsize, int index) {
    ASSERT(*plist, "cannot remove from empty dynlist");

    struct dynlist_header *h = dynlist_header(*plist);

    ASSERT(h->size != 0);
    ASSERT(index < h->size);

    void *data = h + 1;
    memmove(
        data + (index * tsize),
        data + ((index + 1) * tsize),
        (h->size - index - 1) * tsize);

    _dynlist_realloc_impl(plist, tsize, h->size - 1, _DYNLIST_RF_NONE);

    if (*plist) {
        // list may have been removed if requested capacity is 0
        dynlist_header(*plist)->size--;
    }
}

void _dynlist_resize_impl(void **plist, int tsize, int n) {
    if ((!*plist && n != 0)
        || (*plist && dynlist_header(*plist)->capacity < n)) {
        _dynlist_realloc_impl(
            plist, tsize, max(n, DYNLIST_MIN_CAP), _DYNLIST_RF_NONE);
    }

    if (*plist) {
        dynlist_header(*plist)->size = n;
    }
}


void *_dynlist_copy_impl(void **plist, int tsize) {
    if (!*plist) { return NULL; }

    DYNLIST(void) newlist = NULL;

    struct dynlist_header *h = dynlist_header(*plist);
    _dynlist_resize_impl(&newlist, tsize, h->size);
    memcpy(newlist, *plist, tsize * h->size);

    return newlist;
}

void _dynlist_copy_from_impl(void **pdst, void **psrc, int tsize) {
    if (*psrc) {
        struct dynlist_header *h = dynlist_header(*psrc);
        _dynlist_resize_impl(pdst, tsize, h->size);
        memcpy(
            *pdst,
            *psrc,
            h->size * tsize);
    } else {
        _dynlist_free_impl(pdst, tsize);
    }
}
#endif // ifdef CJAM_IMPL
