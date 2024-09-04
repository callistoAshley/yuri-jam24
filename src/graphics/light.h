#pragma once

#include "graphics/graphics.h"
#include "sensible_nums.h"
#include <cglm/struct.h>

typedef struct
{
    vec3s position;
    vec3s color;

    f32 radius;

    QuadEntry quad;
} PointLight;

void point_light_init(PointLight *light, Graphics *graphics, vec3s position,
                      vec3s color, f32 radius);
void point_light_free(PointLight *light, Graphics *graphics);

void point_light_render(PointLight *light, Graphics *graphics,
                        WGPURenderPassEncoder pass, Camera camera);
