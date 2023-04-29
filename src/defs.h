#pragma once

// IMPORTANT: CHANGE BEFORE SUBMIT
// TODO
// XXX
// DO THIS
#define WINDOW_SIZE ((ivec2s) {{ 1280, 720 }})
#define TARGET_SIZE ((ivec2s) {{ 192, 108 }})

#define TILE_SIZE_PX 8

#define LEVEL_WIDTH (192 / 8)
#define LEVEL_HEIGHT (108 / 8)
#define LEVEL_SIZE ((ivec2s) {{ LEVEL_WIDTH, LEVEL_HEIGHT }})

#define Z_MIN -1.0f
#define Z_MAX 1.0f

#define Z_LEVEL_BASE 0.9f
#define Z_LEVEL_OBJECT 0.8f
#define Z_LEVEL_OBJECT_OVERLAY 0.7f
#define Z_UI 0.0f

#define COLOR_WHITE ((vec4s) {{ 1.0f, 1.0f, 1.0f, 1.0f }})
#define COLOR_BLACK ((vec4s) {{ 0.0f, 0.0f, 0.0f, 0.0f }})

