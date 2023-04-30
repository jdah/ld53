#include "level_data.h"

level_data LEVELS[NUM_LEVELS] = {
    [0] = {
        .title = "$35SPECIAL DELIVERY",
        .map = {
            "                   ",
            "    t    x         ",
            "               x   ",
            "    x      x       ",
            "                   ",
            "                   ",
            "SrrrrrrrrrrrrrrrrrF",
            "                   ",
            "        x          ",
            "                t  ",
            "                   ",
            "    x              ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 2 },
        },
        .bonus = 100
    },
    [1] = {
        .title = "$26CURVE IN THE ROAD",
        .map = {
            "                   ",
            "       x       t   ",
            "                   ",
            "     t     x       ",
            "               x   ",
            "        rrrrrrrrrrF",
            "     l  r          ",
            "Srrrrrrrr          ",
            "         l   x     ",
            "     x             ",
            "              t    ",
            "           x       ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [2] = {
        .title = "$42FAT L",
        .map = {
            "        S          ",
            "       xr      t   ",
            "        r          ",
            "     t  r  x       ",
            "        r      x   ",
            "        r          ",
            "        r          ",
            "        r          ",
            "        r    x     ",
            "     x  r          ",
            "        rrrrrrrrrrF",
            "           x       ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [3] = {
        .title = "$55U TURN",
        .map = {
            "  S       F        ",
            "  r    x  r    t   ",
            "  r       r        ",
            "  r  t    rx       ",
            "  r       r    x   ",
            "  r       r        ",
            "  r       r        ",
            "  r       r        ",
            "  r       r  x     ",
            "  r  x    r        ",
            "  rrrrrrrrr        ",
            "           x       ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [4] = {
        .title = "$36EASY MONEY",
        .map = {
            "                   ",
            "     t             ",
            "    x    t     l   ",
            "                   ",
            "xxxxxxxxxxxxxxxxxxx",
            "     t             ",
            "SrrrrrrrrrrrrrrrrrF",
            "              t    ",
            "xxxxxxxxxxxxxxxxxxx",
            "     l             ",
            "                   ",
            "               x   ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [5] = {
        .title = "$50LAKESIDE DRIVE",
        .map = {
            "           F       ",
            "     t     r       ",
            "           r    t  ",
            "     x rrrrr       ",
            "  t    r           ",
            "       rrrrrrrr    ",
            "         lll  r    ",
            "     t   lll  r    ",
            "         lll  r    ",
            "Srrrrrrrrrrrrrr    ",
            "                   ",
            "    t     x     t  ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [6] = {
        .title = "$42WETLANDS",
        .map = {
            "hhh hxhhhhh hhhhxhh",
            "hhhhhhhhhhhhhhhhhhh",
            "Srrrrrhh hhhhh hxhh",
            "hhhhhrhhhhhhhhhhhhh",
            "hhhhhrhhhhh hhhhhhh",
            "h hxhrhhhhrrrrrrrrF",
            "hhhhhrhhxhrhhhxhhhh",
            "hhhhhrrrrrrhhhhhxhh",
            "hh hhhhhhhhhhh hhhh",
            "hhhhhhxhhhhhxhhhxhh",
            "hhh hhhhh hhhhhhhhh",
            "hhhhhhxhhhhhhhh hhh",
            "hhhhhhhhhhhhhhhhhhh",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [7] = {
        .title = "$06STAIR STEPPER",
        .map = {
            "                   ",
            "            x      ",
            "Srrrrr             ",
            "     r             ",
            "     r             ",
            "     rrrrrr        ",
            "          r        ",
            "          r        ",
            "     x    rrrrr    ",
            "              r    ",
            "              r    ",
            "              rrrrF",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [8] = {
        .title = "$50LOOP-DE-LOOP",
        .map = {
            "         x         ",
            "                   ",
            "     rrrrrrrrrr    ",
            "     r        r    ",
            "     r    F   r    ",
            "     rrrrrr   r    ",
            "              r    ",
            "Srrrrrrrrrrrrrr    ",
            "                   ",
            "                   ",
            "                   ",
            "             x     ",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
    [9] = {
        .title = "$36I'M GOING TO\nBE A WHILE...",
        .map = {
            "                   ",
            "          x        ",
            "                   ",
            "Srrrrrrrrrrrrrrrrrr",
            "          x       r",
            "rrrrrrrrrrrrrrrrrrr",
            "r         x        ",
            "rrrrrrrrrrrrrrrrrrr",
            "          x       r",
            "rrrrrrrrrrrrrrrrrrr",
            "r                  ",
            "rrrrrrrrrrrrrrrrrrF",
            "                   ",
        },
        .ships = {
            { ENTITY_SHIP_L0, 4 },
        },
        .bonus = 150
    },
};
