#pragma once

#include "../math.h"

// load png from path, returns 0 on success
int cj_load_png(const char *filepath, u8 **pdata, ivec2s *psize);

#ifdef CJAM_IMPL
int cj_load_png(const char *filepath, u8 **pdata, ivec2s *psize) {
    char *filedata;
    usize filesz;
    ASSERT(
        !file_read(filepath, &filedata, &filesz),
        "failed to read %s",
        filepath);

    SDL_RWops *rw = SDL_RWFromMem(filedata, filesz);
    ASSERT(rw);

    SDL_Surface *surf = IMG_LoadPNG_RW(rw);
    ASSERT(surf);
    ASSERT(surf->w >= 0 && surf->h >= 0);

    if (surf->format->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *newsurf =
            SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surf);
        surf = newsurf;
    }

    *pdata = malloc(surf->w * surf->h * 4);
    *psize = (ivec2s) {{ surf->w, surf->h }};

    for (int y = 0; y < surf->h; y++) {
        memcpy(
            &(*pdata)[y * (psize->x * 4)],
            &((u8*) surf->pixels)[(surf->h - y - 1) * (psize->x * 4)],
            psize->x * 4);
    }

    SDL_FreeSurface(surf);
    CJAM_FREE(filedata);

    return 0;
}
#endif // CJAM_IMPL
