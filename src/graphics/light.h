#pragma once

#include "graphics/graphics.h"
#include "sensible_nums.h"
#include <cglm/struct.h>

typedef struct
{
    enum
    {
        Light_Point,
        Light_Direct
    } type;

    // TODO add spotlights
    union
    {
        struct
        {
            vec2s position;
            f32 radius;
        } point;
    } data;

    vec3s color;

    f32 intensity;
    f32 volumetric_intensity;

    bool casts_shadows;
    ShadowMapEntry shadowmap_entry;
} Light;

void light_render(Light *light, WGPURenderPassEncoder pass, Camera camera);
