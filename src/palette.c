#include "palette.h"

#include <cjam/assert.h>

vec4s palette_get(int i) {
    ASSERT(i >= 0 && i <= 64);
    const u32 argb = palette[i];
    return (vec4s) {{
        ((argb >> 16) & 0xFF) / 255.0f,
        ((argb >> 8) & 0xFF) / 255.0f,
        ((argb >> 0) & 0xFF) / 255.0f,
        ((argb >> 24) & 0xFF) / 255.0f,
    }};
}

u32 palette[64] = {
0xFFff0040,
0xFF131313,
0xFF1b1b1b,
0xFF272727,
0xFF3d3d3d,
0xFF5d5d5d,
0xFF858585,
0xFFb4b4b4,
0xFFffffff,
0xFFc7cfdd,
0xFF92a1b9,
0xFF657392,
0xFF424c6e,
0xFF2a2f4e,
0xFF1a1932,
0xFF0e071b,
0xFF1c121c,
0xFF391f21,
0xFF5d2c28,
0xFF8a4836,
0xFFbf6f4a,
0xFFe69c69,
0xFFf6ca9f,
0xFFf9e6cf,
0xFFedab50,
0xFFe07438,
0xFFc64524,
0xFF8e251d,
0xFFff5000,
0xFFed7614,
0xFFffa214,
0xFFffc825,
0xFFffeb57,
0xFFd3fc7e,
0xFF99e65f,
0xFF5ac54f,
0xFF33984b,
0xFF1e6f50,
0xFF134c4c,
0xFF0c2e44,
0xFF00396d,
0xFF0069aa,
0xFF0098dc,
0xFF00cdf9,
0xFF0cf1ff,
0xFF94fdff,
0xFFfdd2ed,
0xFFf389f5,
0xFFdb3ffd,
0xFF7a09fa,
0xFF3003d9,
0xFF0c0293,
0xFF03193f,
0xFF3b1443,
0xFF622461,
0xFF93388f,
0xFFca52c9,
0xFFc85086,
0xFFf68187,
0xFFf5555d,
0xFFea323c,
0xFFc42430,
0xFF891e2b,
0xFF571c27,
};
