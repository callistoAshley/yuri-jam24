#pragma once

#include "cglm/quat.h"
#include <cglm/cglm.h>

typedef float quat[4];

typedef mat4 Viewport;

typedef struct
{
    vec3 position;
    vec3 scale;
    quat rotation;
} Transform;
