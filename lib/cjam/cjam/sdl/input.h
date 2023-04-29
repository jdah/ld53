#pragma once

#include <SDL.h>

#include "../types.h"
#include "../dynlist.h"
#include "../math.h"

#define INPUT_PRESENT      (1 << 7)
#define INPUT_REPEAT       (1 << 3)
#define INPUT_PRESS        (1 << 2)
#define INPUT_RELEASE      (1 << 1)
#define INPUT_DOWN         (1 << 0)
#define INPUT_INVALID      0

// SDL2 input manager
struct input {
    // TODO
    ivec2 cursor;
    ivec2 cursormotion;

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
};

// initialize input
void input_init(struct input*);

// called each frame
void input_update(struct input*);

// process SDL event (call for each frame for each event after input_update)
void input_process(struct input*, const SDL_Event*);

// get button state for specified string
int input_get(struct input*, const char*);

#ifdef CJAM_IMPL

#include "../time.h"
#include "../assert.h"
#include "../config.h"
#include "../str.h"

void input_init(struct input *input) {
    memset(input, 0, sizeof(*input));
}

void input_update(struct input *input) {
    dynlist_each(input->clear, it) {
        input->buttons[*it.el].state &= ~(INPUT_PRESS | INPUT_RELEASE);
    }
}

void input_process(struct input *input, const SDL_Event *ev) {
    const u64 now = time_ns();
    switch (ev->type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        int i;
        bool down, repeat = false;

        if (ev->type == SDL_KEYDOWN
            || ev->type == SDL_KEYUP) {
            down = ev->key.type == SDL_KEYDOWN;
            repeat = ev->key.repeat;
            i = ev->key.keysym.scancode;
        } else if (
            ev->type == SDL_MOUSEBUTTONDOWN
            || ev->type == SDL_MOUSEBUTTONUP) {
            down = ev->button.type == SDL_MOUSEBUTTONDOWN;
            i = ARRLEN(input->keystate) + (ev->button.button - 1);
        } else {
            ASSERT(false);
        }

        input->buttons[i].time = now;

        const u8 state = input->buttons[i].state;
        u8 newstate = INPUT_PRESENT;

        if (repeat) {
            newstate |= INPUT_DOWN | INPUT_REPEAT;
        }

        if (down) {
            if (!(state & INPUT_DOWN)) {
                newstate |= INPUT_PRESS;
                *dynlist_push(input->clear) = i;
            }

            newstate |= INPUT_DOWN;
        } else {
            if (state & INPUT_DOWN) {
                newstate |= INPUT_RELEASE;
                *dynlist_push(input->clear) = i;
            }
        }

        input->buttons[i].state = newstate;
    } break;
    }
}

// get button state for specified string
static bool get_button_state(
    struct input *p,
    const char *name,
    u8 *pstate,
    u64 *ptime) {
    char buf[256];
    const char *basename = name;

#ifdef CJAM_OSX
#   define METANAME "command"
#elifdef CJAM_WINDOWS
#   define METANAME "windows"
#else
#   define METANAME "gui"
#endif // ifdef CJAM_OSX

    if (!strsuf(name, " meta")) {
        basename = buf;
        snprintf(buf, sizeof(buf), "%s", name);

        char *q = buf;
        while (*q && !isspace(*q)) {
            q++;
        }

        if (!*q) {
            WARN("super invalid keycode %s", name);
            return false;
        }

        *q = '\0';

        xnprintf(buf, sizeof(buf), " %s", METANAME);
    }

    if (!strpre(basename, "mouse_")) {
        int index;
        if (!strsuf(basename, "left"))        { index = 0; }
        else if (!strsuf(basename, "middle")) { index = 1; }
        else if (!strsuf(basename, "right"))  { index = 2; }
        else { return false; }

        if (pstate) { *pstate = p->mousestate[index].state; }
        if (ptime) { *ptime = p->mousestate[index].time; }
    } else {
        const int index = SDL_GetScancodeFromName(basename);
        if (pstate) { *pstate = p->keystate[index].state; }
        if (ptime) { *ptime = p->keystate[index].time; }
    }

    return true;
}

int input_get(struct input *p, const char *name) {
    if (strchr(name, '+')) {
        // this is actually multiple buttons...
        char dup[128];
        int res = snprintf(dup, sizeof(dup), "%s", name);
        if (res < 0 || res > (int) sizeof(dup)) {
            WARN("button name %s is too long", name);
            return INPUT_INVALID;
        }

        bool first = true,
             down_norel = false,
             anypress = false,
             anyrel = false;

        char *lasts;
        for (char *tok = strtok_r(dup, "+", &lasts);
             tok != NULL;
             tok = strtok_r(NULL, "+", &lasts)) {
            u8 state;
            if (!get_button_state(p, tok, &state, NULL)) {
                return INPUT_INVALID;
            }

            if (first) {
                down_norel = !!(state & INPUT_DOWN);
                first = false;
            } else if (!(state & INPUT_RELEASE)) {
                down_norel &= !!(state & INPUT_DOWN);
            }

            anypress |= !!(state & INPUT_PRESS);
            anyrel |= !!(state & INPUT_RELEASE);
        }

        const bool down = !anyrel && down_norel;

        u8 b = INPUT_PRESENT;
        b |= down ? INPUT_DOWN : 0;
        b |= (down && anypress) ? INPUT_PRESS : 0;
        b |= down_norel && anyrel?  INPUT_RELEASE : 0;
        return b;
    } else if (strchr(name, '|')) {
        // this is a union of buttons
        char dup[128];
        int res = snprintf(dup, sizeof(dup), "%s", name);
        if (res < 0 || res > (int) sizeof(dup)) {
            WARN("button name %s is too long", name);
            return INPUT_INVALID;
        }

        u8 state = 0;
        char *lasts;
        for (char *tok = strtok_r(dup, "|", &lasts);
             tok != NULL;
             tok = strtok_r(NULL, "|", &lasts)) {

            u8 s;
            if (!get_button_state(p, tok, &s, NULL)) {
                return INPUT_INVALID;
            }

            state |= s;
        }

        return state;
    }

    u8 state;
    if (!get_button_state(p, name, &state, NULL)) {
        return INPUT_INVALID;
    }

    return state;
}

#endif // ifdef CJAM_IMPL
