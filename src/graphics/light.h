#pragma once

#include "graphics/graphics.h"
#include "sensible_nums.h"
#include <cglm/struct.h>

typedef struct
{
    vec3s position;
    vec3s color;

    f32 intensity;
    f32 radius;
    f32 volumetric_intensity;
    vec2s angle;
} PointLight;

void point_light_init(PointLight *light, vec3s position, vec3s color,
                      f32 radius);
void point_light_free(PointLight *light);

void point_light_render(PointLight *light, WGPURenderPassEncoder pass,
                        Camera camera);

typedef struct
{
    vec3s color;
    f32 angle;
} DirectionalLight;

void directional_light_init(DirectionalLight *light, vec3s color, f32 angle);
void directional_light_free(DirectionalLight *light);

void directional_light_render(DirectionalLight *light,
                              WGPURenderPassEncoder pass);
