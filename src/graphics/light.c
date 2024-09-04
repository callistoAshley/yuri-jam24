#include "light.h"
#include "core_types.h"
#include "graphics/graphics.h"
#include "graphics/quad_manager.h"
#include "graphics/shaders.h"
#include "utility/common_defines.h"

void point_light_init(PointLight *light, Graphics *graphics, vec3s position,
                      vec3s color, f32 radius)
{
    Rect rect =
        rect_centered_from_size((vec2s){.x = radius * 2, .y = radius * 2});
    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);

    Quad quad = quad_init(rect, tex_coords);
    QuadEntry entry = quad_manager_add(&graphics->quad_manager, quad);

    light->position = position;
    light->color = color;
    light->radius = radius;
    light->quad = entry;
}

void point_light_free(PointLight *light, Graphics *graphics)
{
    quad_manager_remove(&graphics->quad_manager, light->quad);
}

void point_light_render(PointLight *light, Graphics *graphics,
                        WGPURenderPassEncoder pass, Camera camera)
{
    f32 internal_sale =
        (f32)graphics->wgpu.surface_config.width / INTERNAL_SCREEN_WIDTH;
    LightPushConstants push_constants = {
        .color = light->color,

        .position = (vec2s){.x = light->position.x, .y = light->position.y},
        .camera_position = (vec2s){.x = camera.x, .y = camera.y},

        .radius = light->radius,
        .internal_scale = internal_sale,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(LightPushConstants), &push_constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD, 1,
                              QUAD_ENTRY_TO_VERTEX_INDEX(light->quad), 0);
}
