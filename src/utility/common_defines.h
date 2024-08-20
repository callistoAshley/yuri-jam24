#pragma once

#define TILE_SIZE 8

#define TILES_PER_SCREEN_WIDTH INTERNAL_SCREEN_WIDTH / TILE_SIZE
#define TILES_PER_SCREEN_HEIGHT INTERNAL_SCREEN_HEIGHT / TILE_SIZE

#define WINDOW_SCALE 8
#define WINDOW_WIDTH (INTERNAL_SCREEN_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (INTERNAL_SCREEN_HEIGHT * WINDOW_SCALE)

// we use 64hz as the fixed time step (see bevy's reasoning)
#define FIXED_STEPS_PER_SEC 64

// pixels per meter
// one tile (8px) is one meter
#define PX_PER_M TILE_SIZE
// convert between pixels and meters
#define PX_TO_M(x) ((x) / PX_PER_M)
// convert between meters and pixels
#define M_TO_PX(x) ((x) * PX_PER_M)

#define INTERNAL_SCREEN_WIDTH 160
#define INTERNAL_SCREEN_HEIGHT 90
