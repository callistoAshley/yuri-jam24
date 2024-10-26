#include "definition.h"

// while this is a gnu C extension clang will compile it just fine
// (MSVC won't but MSVC doesn't even do C99. also fuck microsoft)
#ifdef __clang__
#pragma clang diagnostic ignored "-Wgnu-flexible-array-initializer"
#endif

static const AnimationDef TEST = {
    .name = "test",
    .type = Anim_Test,

    .cell_width = 8,
    .cell_height = 8,

    .looping = true,

    .cell_count = 5,
    .frames =
        {
            {0.25, 0},
            {0.25, 1},
            {0.25, 2},
            {0.25, 3},
            {0.25, 4},
        },
};

const AnimationDef *ANIMATIONS[Anim_Max] = {
    [Anim_Test] = &TEST,
};

Rect tex_coords_for(AnimationType type, u32 frame, u32 texture_width,
                    u32 texture_height)
{
    const AnimationDef *animation = ANIMATIONS[type];

    Cell cell = animation->frames[frame].cell;

    u32 unwrapped_cell_x = cell * animation->cell_width;
    // we want the cell to wrap around to the start of the texture and go down a
    // row for every wrap
    u32 cell_x = unwrapped_cell_x % texture_width;
    u32 cell_y = (unwrapped_cell_x / texture_width) * animation->cell_height;

    vec2s tex_size = {.x = texture_width, .y = texture_height};
    vec2s cell_size = {.x = animation->cell_width, .y = animation->cell_height};

    vec2s min = {.x = cell_x, .y = cell_y};
    vec2s max = glms_vec2_add(min, cell_size);

    // normalize texture coordinates to 0,1
    Rect rect = {.min = glms_vec2_div(min, tex_size),
                 .max = glms_vec2_div(max, tex_size)};
    return rect;
}
