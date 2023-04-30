#include "level_data.h"

level_data LEVELS[NUM_LEVELS] = {
    [0] = {
        .map = {
            "              F    ",
            "              r    ",
            "         x    r    ",
            "   x          r    ",
            "        rrrrrrr    ",
            "        r          ",
            "        r   x      ",
            "Srrrrrrrr          ",
            "               x   ",
            "        x          ",
            "    x          x   ",
            "                   ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 1 }
        }
    }
};
