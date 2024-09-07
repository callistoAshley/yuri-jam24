#include "light.h"
#include "core_types.h"
#include "graphics/graphics.h"
#include "graphics/quad_manager.h"
#include "graphics/shaders.h"
#include "utility/common_defines.h"

void point_light_init(PointLight *light, vec3s position, vec3s color,
                      f32 radius)
{
    light->position = position;
    light->color = color;
    light->radius = radius;
}

void point_light_free(PointLight *light)
{
    // currently a no-op
    (void)light;
}

void point_light_render(PointLight *light, WGPURenderPassEncoder pass,
                        Camera camera)
{
    LightPushConstants push_constants = {
        .color = light->color,

        .position = (vec2s){.x = light->position.x, .y = light->position.y},
        .camera_position = (vec2s){.x = camera.x, .y = camera.y},

        .radius = light->radius,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(LightPushConstants), &push_constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);
}
