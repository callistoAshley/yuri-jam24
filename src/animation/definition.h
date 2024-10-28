#pragma once

#include "core_types.h"
#include "sensible_nums.h"

typedef enum
{
    Anim_Test = 0,

    Anim_Max,
} AnimationType;

typedef u32 AnimCell;
typedef struct
{
    f32 frame_time;
    AnimCell cell;
} Frame;

typedef struct
{
    const char *name;
    AnimationType type;

    bool looping;

    u32 cell_width, cell_height;

    u32 frame_count;
    Frame frames[];
} AnimationDef;

extern const AnimationDef *ANIMATIONS[Anim_Max];

AnimationType anim_type_for(const char *name);

Rect tex_coords_for(const AnimationDef *def, u32 frame, u32 texture_width,
                    u32 texture_height);
