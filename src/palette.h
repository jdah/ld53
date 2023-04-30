#pragma once

#include <cjam/types.h>
#include <cjam/math.h>

#define PALETTE_WHITE 8
#define PALETTE_YELLOW 33
#define PALETTE_RED 61
#define PALETTE_BLACK 0
#define PALETTE_LIGHT_GRAY 7
#define PALETTE_LIGHT_BLUE 45
#define PALETTE_ORANGE 30

#define PALETTE_L0 6
#define PALETTE_L1 12
#define PALETTE_L2 62

#define PALETTE_ALIEN_GREEN 36
#define PALETTE_ALIEN_GREY 6
#define PALETTE_ALIEN_RED 61

#define PALETTE_SKY 40

vec4s palette_get(int i);

extern u32 palette[64];
