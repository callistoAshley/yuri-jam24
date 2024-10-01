#pragma once

#include "caster_manager.h"
#include "graphics/transform_manager.h"

typedef struct
{
    TransformEntry transform;
    CasterEntry *caster;
    u32 cell;
} ShadowCaster;
