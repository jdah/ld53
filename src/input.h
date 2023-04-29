#pragma once

#include <SDL.h>

#include <cjam/types.h>
#include <cjam/dynlist.h>
#include <cjam/math.h>

#define INPUT_PRESENT      (1 << 7)
#define INPUT_REPEAT       (1 << 3)
#define INPUT_PRESS        (1 << 2)
#define INPUT_RELEASE      (1 << 1)
#define INPUT_DOWN         (1 << 0)
#define INPUT_INVALID      0

// SDL2 input manager
typedef struct {
    // TODO
    struct {
        ivec2s pos;
        ivec2s delta;
        ivec2s last_pos;
        ivec2s tile;
        ivec2s tile_px;
        bool in_level;
    } cursor;

    union {
        struct {
            struct { u8 state; u64 time; }
                keystate[SDL_NUM_SCANCODES],
                mousestate[3];
        };

        struct { u8 state; u64 time; } buttons[SDL_NUM_SCANCODES + 3];
    };

    // buttons which must have PRESS/RELEASE reset
    DYNLIST(int) clear;
} input;

// initialize input
void input_init(input*);

// called each frame
void input_update(input*);

// process SDL event (call for each frame for each event after input_update)
void input_process(input*, const SDL_Event*);

// get button state for specified string
int input_get(input*, const char*);
