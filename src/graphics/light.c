#include "light.h"
#include "core_types.h"
#include "graphics/graphics.h"
#include "graphics/quad_manager.h"
#include "graphics/shaders.h"
#include "graphics/shadowmap.h"
#include "utility/common_defines.h"

void light_render(Light *light, WGPURenderPassEncoder pass, Camera camera)
{
    switch (light->type)
    {
    case Light_Point:
    {
        PointLightPushConstants push_constants = {
            .color = light->color,

            .position = light->data.point.position,
            .camera_position = (vec2s){.x = camera.x, .y = camera.y},

            .radius = light->data.point.radius,
            .intensity = light->intensity,
            .volumetric_intensity = light->volumetric_intensity,
        };

        if (light->casts_shadows)
        {
            vec2s mask_offset =
                SHADOWMAP_ENTRY_POS_OFFSET(light->shadowmap_entry);
            // z = 1.0 to indicate that yes, this does cast shadows
            push_constants.mask_tex_offset =
                (vec3s){.x = mask_offset.x, .y = mask_offset.y, .z = 1.0};
        }

        wgpuRenderPassEncoderSetPushConstants(
            pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
            sizeof(PointLightPushConstants), &push_constants);
        wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);

        break;
    }
    case Light_Direct:
    {
        DirectLightPushConstants push_constants = {
            .color = light->color,
            .intensity = light->intensity,
            .volumetric_intensity = light->volumetric_intensity,
        };

        if (light->casts_shadows)
        {
            vec2s mask_offset =
                SHADOWMAP_ENTRY_POS_OFFSET(light->shadowmap_entry);
            // z = 1.0 to indicate that yes, this does cast shadows
            push_constants.mask_tex_offset =
                (vec3s){.x = mask_offset.x, .y = mask_offset.y, .z = 1.0};
        }

        wgpuRenderPassEncoderSetPushConstants(
            pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
            sizeof(DirectLightPushConstants), &push_constants);
        wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);

        break;
    }
    }
}
