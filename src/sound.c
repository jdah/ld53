#include "sound.h"
#include "util.h"

#include "../lib/soloud/include/soloud_c.h"

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

static Soloud *soloud;

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
        soloud = Soloud_create();
        Soloud_init(soloud);
    }

    return true;
}

void sound_play(const char *resource, f32 volume) {
    if (!ensure_init()) { return; }

    static bool init;

    // char* -> Wav*
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

    Wav *wav = Wav_create();
    char fullpath[1024];
    resource_to_path(fullpath, sizeof(fullpath), resource);
    Wav_load(wav, fullpath);

    Wav
        **psrc = map_find(Wav*, &sounds, resource),
        *src = NULL;

    if (psrc) {
        src = *psrc;
    } else {
        char fullpath[1024];
        resource_to_path(fullpath, sizeof(fullpath), resource);

        src = Wav_create();
        Wav_load(wav, fullpath);
        if (!src) {
            WARN("could not create audio for %s", fullpath);
        } else {
            LOG("loaded new sound %s", fullpath);
        }

        map_insert(&sounds, strdup(resource), src);
    }

    const u32 id = Soloud_play(soloud, wav);
    Soloud_setVolume(soloud, id, volume);
}
