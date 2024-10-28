#include "definition.h"
#include "utility/macros.h"
#include <string.h>

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

    .frame_count = 5,
    .frames =
        {
            {1.0, 0, "event:/sfx/clang"},
            {0.25, 1, NULL},
            {0.25, 2, NULL},
            {0.25, 3, NULL},
            {0.25, 4, NULL},
        },
};

const AnimationDef *ANIMATIONS[Anim_Max] = {
    [Anim_Test] = &TEST,
};

AnimationType anim_type_for(const char *name)
{
    for (AnimationType type = 0; type < Anim_Max; type++)
    {
        const AnimationDef *def = ANIMATIONS[type];
        if (!strcmp(def->name, name))
        {
            return type;
        }
    }
    FATAL("Unrecognized animation type %s\n", name);
}

Rect tex_coords_for(const AnimationDef *def, u32 frame, u32 texture_width,
                    u32 texture_height)
{
    AnimCell cell = def->frames[frame].cell;

    u32 unwrapped_cell_x = cell * def->cell_width;
    // we want the cell to wrap around to the start of the texture and go down a
    // row for every wrap
    u32 cell_x = unwrapped_cell_x % texture_width;
    u32 cell_y = (unwrapped_cell_x / texture_width) * def->cell_height;

    vec2s tex_size = {.x = texture_width, .y = texture_height};
    vec2s cell_size = {.x = def->cell_width, .y = def->cell_height};

    vec2s min = {.x = cell_x, .y = cell_y};
    vec2s max = glms_vec2_add(min, cell_size);

    // normalize texture coordinates to 0,1
    Rect rect = {.min = glms_vec2_div(min, tex_size),
                 .max = glms_vec2_div(max, tex_size)};
    return rect;
}
