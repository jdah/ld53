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
            { ENTITY_SHIP_L0, 3 }
        },
        .bonus = 100
    }
};
