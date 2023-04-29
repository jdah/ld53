#pragma once

#include "config.h"

int cj_sound_init();

#ifdef CJAM_IMPL

#include "assert.h"

#pragma clang attribute push (__attribute__((no_sanitize("undefined"))), apply_to=function)
#define CUTE_SOUND_SCALAR_MODE
#define CUTE_SOUND_IMPLEMENTATION
#include <cute_sound.h>
#pragma clang attribute pop

int cj_sound_init() {
    cs_error_t err = cs_init(NULL, 44100, 1, NULL);
    ASSERT(!err, "cute sound error: %s", cs_error_as_string(err));
    return 0;
}

#endif // ifdef CJAM_IMPL
