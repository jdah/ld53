#include "sound.h"
#include "util.h"

#include "../lib/Simple-SDL2-Audio/src/audio.h"
#include "../lib/Simple-SDL2-Audio/src/audio.c"

/* void sound_play(const char *resource) { */
/*     static bool init = false; */

/*     if (!init) { */
/*         init = true; */
/*         initAudio(); */
/*     } */

/*     char fullpath[1024]; */
/*     resource_to_path(fullpath, sizeof(fullpath), resource); */
/*     playSound(fullpath, SDL_MIX_MAXVOLUME); */
/* } */

#include <cjam/assert.h>
#include <cjam/map.h>

#pragma clang attribute push (__attribute__((no_sanitize("undefined"))), apply_to=function)
#define CUTE_SOUND_SCALAR_MODE
#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SOUND_FORCE_SDL
#include <cute_sound.h>
#pragma clang attribute pop

#ifdef EMSCRIPTEN
#include "emscripten.h"
#endif

static bool ensure_init() {
    static bool init;

    if (init) { return true; }

#ifdef EMSCRIPTEN
    const int res = EM_ASM_INT({
        try {
            return new AudioContext().state == 'running' ? 0 : 1;
        } catch {
            return 1;
        }
        return 0;
    });
    if (!res) {
        return false;
    }
#endif

    if (!init) {
        init = true;
        initAudio();
    }

    return true;
}

void sound_play(const char *resource) {
    if (!ensure_init()) { return; }

    static bool init;

    // char* -> cs_audio-source_t*
    static struct map sounds;

    if (!init) {
        init = true;
        map_init(
            &sounds,
            map_hash_str,
            NULL,
            NULL,
            map_dup_str,
            map_cmp_str,
            map_default_free,
            NULL,
            NULL);
    }

    Audio
        **psrc = map_find(Audio*, &sounds, resource),
        *src = NULL;

    if (psrc) {
        src = *psrc;
    } else {
        char fullpath[1024];
        resource_to_path(fullpath, sizeof(fullpath), resource);

        src = createAudio(fullpath, 0, 0);
        if (!src) {
            WARN("could not create audio for %s", fullpath);
        } else {
            LOG("loaded new sound %s", src);
        }

        map_insert(&sounds, strdup(resource), src);
    }

        char fullpath[1024];
        resource_to_path(fullpath, sizeof(fullpath), resource);
    playSound(fullpath, SDL_MIX_MAXVOLUME / 2);
    /* playSoundFromMemory(src, SDL_MIX_MAXVOLUME / 2); */

    /* static bool first; */
    /* static cs_playing_sound_t firstsound; */
    /* ASSERT(src); */
    /* cs_set_global_volume(1.0f); */
    /* cs_set_global_pause(false); */
    /* cs_playing_sound_t playing = */
    /*     cs_play_sound( */
    /*         src, */
    /*         (cs_sound_params_t) { */
    /*             .volume = 1.0f, */
    /*             .delay = 0, */
    /*             .looped = false, */
    /*             .pan = 0.5f, */
    /*             .paused = false */
    /*         }); */
    /* if (!first) { first = true; firstsound = playing; } */
    /* cs_sound_set_is_paused(playing, false); */
    /* LOG("%" PRIu64, cs_sound_get_sample_index(firstsound)); */
}
