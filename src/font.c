#include "font.h"
#include "gfx.h"
#include "palette.h"
#include "state.h"

#define GLYPH_SIZE ((ivec2s) {{ 8, 8 }})

void font_char(
    ivec2s pos,
    f32 z,
    vec4s col,
    int flags,
    char c) {
    if (flags & FONT_DOUBLED) {
        const f32 diff = -0.4f;
        font_char(
            (ivec2s) {{ pos.x + 1, pos.y - 1 }},
            z + 0.00001f,
            (vec4s) {{
                clamp(col.r + diff, 0.0f, 1.0f),
                clamp(col.g + diff, 0.0f, 1.0f),
                clamp(col.b + diff, 0.0f, 1.0f),
                col.a
            }},
            flags & ~FONT_DOUBLED,
            c);
    }

    const ivec2s n = {{
        state->atlas.font.size_px.x / GLYPH_SIZE.x,
        state->atlas.font.size_px.y / GLYPH_SIZE.y
    }};
    gfx_batcher_push_sprite(
        &state->batcher,
        &state->atlas.font,
        &(gfx_sprite) {
            .pos = {{ pos.x, pos.y }},
            .color = col,
            .index = {{ c % n.x, n.y - (c / n.y) - 1 }},
            .z = z,
            .flags = GFX_NO_FLAGS
        });
}


static const char *nextch(
    const char *str,
    char *ch,
    vec4s *col) {
    // interpret '$xx' color control codes
    while (*str == '$'
        && *(str + 1)) {
        if (*(str + 1) == '$') {
            // skip first '$', return '$'
            str++;
            break;
        } else {
            if (!*(str + 2)) {
                WARN("bad color control %s", str);
                return NULL;
            }

            char cs[2] = { *(str + 1), *(str + 2) };
            u8 newcol = 0;

            for (int i = 0; i < 2; i++) {
                if (cs[i] < '0' || cs[i] > '9') {
                    WARN("bad control color %s", str);
                    return NULL;
                }
            }

            newcol = (cs[0] - '0') * 10 + (cs[1] - '0');

            // skip $, x, x
            str += 3;

            *col = palette_get(newcol);
        }
    }

    *ch = *str;
    return str + 1;
}

void font_str(
    ivec2s pos,
    f32 z,
    vec4s col,
    int flags,
    const char *str) {
    int i = 0, j = 0;
    char c;
    while ((str = nextch(str, &c, &col)) && c) {
        if (c == '\n') {
            i = 0;
            j++;
        } else {
            font_char(
                (ivec2s) {{
                    pos.x + (i * GLYPH_SIZE.x),
                    pos.y - (j * GLYPH_SIZE.y)
                }},
                z,
                col,
                flags,
                c);
            i++;
        }
    }
}

void font_v(
    ivec2s pos,
    f32 z,
    vec4s col,
    int flags,
    const char *fmt,
    ...) {
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    font_str(pos, z, col, flags, buf);
}

int font_width(const char *str) {
    int i = 0, w = 0;
    char c;
    vec4s col;
    while ((str = nextch(str, &c, &col)) && c) {
        if (c == '\n') {
            w = max(w, i);
            i = 0;
        } else {
            i++;
        }
    }
    return max(w, i) * GLYPH_SIZE.x;
}

int font_len(const char *str) {
    int i = 0;
    char c;
    vec4s col;
    while ((str = nextch(str, &c, &col)) && c) {
        i++;
    }
    return i;
}

