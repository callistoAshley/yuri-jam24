#include "light.h"
#include "core_types.h"
#include "graphics/graphics.h"
#include "graphics/shaders.h"
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

        Rect screen_rect = rect_from_size(
            (vec2s){.x = GAME_VIEW_WIDTH, .y = GAME_VIEW_HEIGHT});

        Rect light_rect = rect_from_center_radius(
            glms_vec2_sub(push_constants.position,
                          push_constants.camera_position),
            VEC2_SPLAT(push_constants.radius + 1));

        Rect clipped_rect = rect_clip(screen_rect, light_rect);
        if (rect_width(clipped_rect) == 0 || rect_height(clipped_rect) == 0)
            return;

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

        wgpuRenderPassEncoderSetPushConstants(
            pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
            sizeof(DirectLightPushConstants), &push_constants);
        wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1, 0, 0);

        break;
    }
    }
}
