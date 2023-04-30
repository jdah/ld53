#include "level_data.h"

level_data LEVELS[NUM_LEVELS] = {
    [0] = {
        .title = "$35SPECIAL DELIVERY",
        .map = {
            "              F    ",
            "   mmm        r    ",
            "         x    r    ",
            "   x    lllll r    ",
            "  hhh   rrrrrrr    ",
            "  ddd  mrl         ",
            "      mmr   x  t   ",
            "Srrrrrrrrl         ",
            "  t            x   ",
            "        x   t      ",
            "    x          x   ",
            "                   ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 2 },
        },
        .bonus = 100
    },
    [1] = {
        .title = "$34A SIMPLE JOB",
        .map = {
            "                   ",
            "       x           ",
            "                   ",
            "           x       ",
            "               x   ",
            "     l             ",
            "        rrrrrrrrrrF",
            "Srrrrrrrr          ",
            "         l   x     ",
            "     x             ",
            "                   ",
            "           x       ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    }
};
