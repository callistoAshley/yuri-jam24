#include "light.h"
#include "core_types.h"
#include "graphics/graphics.h"
#include "graphics/quad_manager.h"
#include "graphics/shaders.h"
#include "graphics/shadowmap.h"
#include "utility/common_defines.h"

void point_light_render(PointLight *light, WGPURenderPassEncoder pass,
                        Camera camera)
{
    PointLightPushConstants push_constants = {
        .color = light->color,

        .position = light->position,
        .camera_position = (vec2s){.x = camera.x, .y = camera.y},

        .radius = light->radius,
        .intensity = light->intensity,
        .volumetric_intensity = light->volumetric_intensity,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(PointLightPushConstants), &push_constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);
}

void directional_light_render(DirectionalLight *light,
                              WGPURenderPassEncoder pass)
{
    DirectLightPushConstants push_constants = {
        .color = light->color,
        .intensity = light->intensity,
        .volumetric_intensity = light->volumetric_intensity,
    };

    if (light->casts_shadows)
    {
        vec2s mask_offset = SHADOWMAP_ENTRY_POS_OFFSET(light->shadowmap_entry);
        // z = 1.0 to indicate that yes, this does cast shadows
        push_constants.mask_tex_offset =
            (vec3s){.x = mask_offset.x, .y = mask_offset.y, .z = 1.0};
    }

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(DirectLightPushConstants), &push_constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);
}
