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
            { ENTITY_SHIP_L2, 1 },
            { ENTITY_SHIP_L2, 1 }
        },
        .bonus = 100
    }
};
