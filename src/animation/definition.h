#pragma once

#include "core_types.h"
#include "sensible_nums.h"

typedef enum
{
    Anim_Test = 0,

    Anim_Max,
} AnimationType;

typedef u32 Cell;
typedef struct
{
    f32 frame_time;
    Cell cell;
} Frame;

typedef struct
{
    const char *name;
    AnimationType type;

    bool looping;

    u32 cell_width, cell_height;

    u32 cell_count;
    Frame frames[];
} AnimationDef;

extern const AnimationDef *ANIMATIONS[Anim_Max];

Rect tex_coords_for(AnimationType type, u32 frame, u32 texture_width,
                    u32 texture_height);
