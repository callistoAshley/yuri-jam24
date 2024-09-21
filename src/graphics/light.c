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
    light->intensity = 1.5f;
    light->volumetric_intensity = 0.1f;
    light->angle = (vec2s){.x = TO_RAD(-180.0), .y = TO_RAD(180.0)};
}

void point_light_free(PointLight *light)
{
    // currently a no-op
    (void)light;
}

void point_light_render(PointLight *light, WGPURenderPassEncoder pass,
                        Camera camera)
{
    PointLightPushConstants push_constants = {
        .color = light->color,

        .position = (vec2s){.x = light->position.x, .y = light->position.y},
        .camera_position = (vec2s){.x = camera.x, .y = camera.y},

        .radius = light->radius,
        .intensity = light->intensity,
        .volumetric_intensity = light->volumetric_intensity,
        .angle = light->angle,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(PointLightPushConstants), &push_constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);
}

void directional_light_init(DirectionalLight *light, vec3s color, f32 angle)
{
    light->color = color;
    light->angle = angle;
}

void directional_light_free(DirectionalLight *light)
{
    // currently a no-op
    (void)light;
}

void directional_light_render(DirectionalLight *light,
                              WGPURenderPassEncoder pass)
{
    DirectLightPushConstants push_constants = {
        .color = light->color,
        .angle = light->angle,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(DirectLightPushConstants), &push_constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);
}
