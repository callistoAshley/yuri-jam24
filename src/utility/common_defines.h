#pragma once

#define TILE_SIZE 8

#define TILES_PER_SCREEN_WIDTH GAME_VIEW_WIDTH / TILE_SIZE
#define TILES_PER_SCREEN_HEIGHT GAME_VIEW_HEIGHT / TILE_SIZE

#define UI_SCALE 8
#define UI_VIEW_WIDTH (GAME_VIEW_WIDTH * UI_SCALE)
#define UI_VIEW_HEIGHT (GAME_VIEW_HEIGHT * UI_SCALE)

#define BASE_WINDOW_WIDTH UI_VIEW_WIDTH
#define BASE_WINDOW_HEIGHT UI_VIEW_HEIGHT

// pixels per meter
// one tile (8px) is one meter
#define PX_PER_M TILE_SIZE
// convert between pixels and meters
#define PX_TO_M(x) ((x) / PX_PER_M)
// convert between meters and pixels
#define M_TO_PX(x) ((x) * PX_PER_M)

#define GAME_VIEW_WIDTH 160
#define GAME_VIEW_HEIGHT 90

#define VEC3_SPLAT(val)                                                        \
    (vec3s) { .x = val, .y = val, .z = val }
#define VEC2_SPLAT(val)                                                        \
    (vec2s) { .x = val, .y = val }

#define TEXTURE_PATH(path) "assets/textures/" path

#define TO_RAD(x) ((x) / 180.0f * M_PI)
#define TO_DEG(x) ((x) / M_PI * 180.0f)
