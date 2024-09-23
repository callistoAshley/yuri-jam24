#include "shaders.h"
#include "core_types.h"
#include "graphics/bind_group_layouts.h"
#include "graphics/wgpu_resources.h"
#include "sensible_nums.h"
#include "utility/macros.h"
#include "utility/files.h"
#include "webgpu.h"

#include <wgpu.h>
#include <stdio.h>
#include <libgen.h>

#define PUSH_CONSTANTS_FOR(type)                                               \
    {                                                                          \
        (WGPUPushConstantRange){                                               \
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,       \
            .start = 0,                                                        \
            .end = sizeof(type),                                               \
        },                                                                     \
    }

WGPURenderPipeline
create_shader(const char *path, const char *label, WGPUBindGroupLayout *layouts,
              u32 layout_count, WGPUPushConstantRange *push_constant_ranges,
              u32 push_constant_range_count, WGPUVertexBufferLayout *buffers,
              u32 buffer_count, WGPUColorTargetState *color_targets,
              u32 target_count, WGPUDepthStencilState *depth_state,
              WGPUPrimitiveState *primitive_state, WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file(path, &buf, &buf_len);
    if (!buf)
        FATAL("failed to read shader file %s", path);

    // this uses null-terminated strings, but read_entire_file returns something
    // null-terminated anyway, so it's fine (really wish this used a
    // length-based thing instead!!)
    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor module_descriptor = {
        .label = label,
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = push_constant_range_count,
    };
    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = label,
        .bindGroupLayoutCount = layout_count,
        .bindGroupLayouts = layouts,
    };
    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexState vertex_state = {.module = module,
                                    .entryPoint = "vs_main",
                                    .buffers = buffers,
                                    .bufferCount = buffer_count};

    WGPUFragmentState fragment_state = {.module = module,
                                        .entryPoint = "fs_main",
                                        .targetCount = target_count,
                                        .targets = color_targets};
    WGPUFragmentState *fragment = &fragment_state;
    if (target_count == 0)
        fragment = NULL;

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };
    if (primitive_state)
        primitive = *primitive_state;

    // Antialiasing is for losers
    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = label,
        .layout = layout,
        .vertex = vertex_state,
        .fragment = fragment,
        .primitive = primitive,
        .multisample = multisample,
        .depthStencil = depth_state,
    };
    WGPURenderPipeline pipeline =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);
    PTR_ERRCHK(pipeline, "failed to create render pipeline");

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);

    return pipeline;
}

void shaders_init(Shaders *shaders, BindGroupLayouts *layouts,
                  WGPUResources *resources)
{
    // vertex layout if the shader uses the quad vertex buffer
    WGPUVertexAttribute quad_vertex_attributes[] = {
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        },
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = sizeof(vec2s),
            .shaderLocation = 1,
        }};
    WGPUVertexBufferLayout quad_vertex_buffer_layout = {
        .arrayStride = sizeof(Vertex),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 2,
        .attributes = quad_vertex_attributes};

    // color targets used for defferred rendering
    WGPUColorTargetState defferred_targets[] = {(WGPUColorTargetState){
        .format = WGPUTextureFormat_RGBA8Unorm,
        .writeMask = WGPUColorWriteMask_All,
    }};

    // targets for rendering directly to the window
    WGPUColorTargetState surface_targets[] = {(WGPUColorTargetState){
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
    }};

    // perform additive blending
    WGPUBlendComponent additive_components = {
        .srcFactor = WGPUBlendFactor_One,
        .dstFactor = WGPUBlendFactor_One,
        .operation = WGPUBlendOperation_Add,
    };
    WGPUBlendState additive_blend = {
        .color = additive_components,
        .alpha = additive_components,
    };
    WGPUColorTargetState additive_surface_targets[] = {(WGPUColorTargetState){
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
        .blend = &additive_blend,
    }};

    // perform regular alpha blending
    WGPUBlendComponent alpha_color = {
        .srcFactor = WGPUBlendFactor_SrcAlpha,
        .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
        .operation = WGPUBlendOperation_Add,
    };
    WGPUBlendComponent alpha_alpha = {
        .srcFactor = WGPUBlendFactor_One,
        .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
        .operation = WGPUBlendOperation_Add,
    };
    WGPUBlendState alpha_blend = {
        .color = alpha_color,
        .alpha = alpha_alpha,
    };
    WGPUColorTargetState alpha_surface_targets[] = {(WGPUColorTargetState){
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
        .blend = &alpha_blend,
    }};

    WGPUColorTargetState shadowmap_targets[] = {(WGPUColorTargetState){
        .format = WGPUTextureFormat_R8Unorm,
        .writeMask = WGPUColorWriteMask_All,
    }};

    WGPUPushConstantRange sprite_constants[] =
        PUSH_CONSTANTS_FOR(SpritePushConstants);
    shaders->defferred.sprite =
        create_shader("assets/shaders/sprite.wgsl", "sprite", &layouts->sprite,
                      1, sprite_constants, 1, &quad_vertex_buffer_layout, 1,
                      defferred_targets, 1, NULL, NULL, resources);

    WGPUPushConstantRange ui_sprite_constants[] =
        PUSH_CONSTANTS_FOR(UiSpritePushConstants);
    shaders->forward.ui_sprite = create_shader(
        "assets/shaders/ui_sprite.wgsl", "ui_sprite", &layouts->sprite, 1,
        ui_sprite_constants, 1, &quad_vertex_buffer_layout, 1,
        alpha_surface_targets, 1, NULL, NULL, resources);

    WGPUVertexAttribute tilemap_vertex_attributes[] = {(WGPUVertexAttribute){
        .format = WGPUVertexFormat_Sint32,
        .offset = 0,
        .shaderLocation = 0,
    }};
    WGPUVertexBufferLayout tilemap_vertex_buffer_layout = {
        .arrayStride = sizeof(u32),
        .stepMode = WGPUVertexStepMode_Instance,
        .attributeCount = 1,
        .attributes = tilemap_vertex_attributes,
    };
    WGPUPushConstantRange tilemap_constants[] =
        PUSH_CONSTANTS_FOR(TilemapPushConstants);

    shaders->defferred.tilemap = create_shader(
        "assets/shaders/tilemap.wgsl", "tilemap", &layouts->tilemap, 1,
        tilemap_constants, 1, &tilemap_vertex_buffer_layout, 1,
        defferred_targets, 1, NULL, NULL, resources);

    WGPUPushConstantRange light_constants[] =
        PUSH_CONSTANTS_FOR(PointLightPushConstants);
    shaders->lights.point =
        create_shader("assets/shaders/point_light.wgsl", "point_light",
                      &layouts->lighting, 1, light_constants, 1, NULL, 0,
                      additive_surface_targets, 1, NULL, NULL, resources);

    WGPUPushConstantRange direct_constants[] =
        PUSH_CONSTANTS_FOR(DirectLightPushConstants);
    shaders->lights.direct =
        create_shader("assets/shaders/direct_light.wgsl", "direct_light",
                      &layouts->lighting, 1, direct_constants, 1, NULL, 0,
                      additive_surface_targets, 1, NULL, NULL, resources);

    shaders->forward.screen_blit = create_shader(
        "assets/shaders/screen_blit.wgsl", "screen_blit", &layouts->screen_blit,
        1, NULL, 0, NULL, 0, surface_targets, 1, NULL, NULL, resources);

    WGPUPushConstantRange b2d_circle_constants[] =
        PUSH_CONSTANTS_FOR(B2DCirclePushConstants);
    shaders->box2d_debug.circle =
        create_shader("assets/shaders/b2d_circle.wgsl", "b2d_circle", NULL, 0,
                      b2d_circle_constants, 1, &quad_vertex_buffer_layout, 1,
                      alpha_surface_targets, 1, NULL, NULL, resources);

    WGPUVertexAttribute b2d_polygon_attributes[] = {(WGPUVertexAttribute){
        .format = WGPUVertexFormat_Float32x2,
        .offset = 0,
        .shaderLocation = 0,
    }};
    WGPUVertexBufferLayout b2d_buffer_layout = {
        .arrayStride = sizeof(vec2s),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 1,
        .attributes = b2d_polygon_attributes};

    WGPUPushConstantRange b2d_polygon_push_constants[] =
        PUSH_CONSTANTS_FOR(B2DDrawPolygonPushConstants);
    shaders->box2d_debug.polygon =
        create_shader("assets/shaders/b2d_polygon.wgsl", "b2d_polygon", NULL, 0,
                      b2d_polygon_push_constants, 1, &b2d_buffer_layout, 1,
                      alpha_surface_targets, 1, NULL, NULL, resources);

    WGPUVertexAttribute shadowmap_vertex_attributes[] = {(WGPUVertexAttribute){
        .format = WGPUVertexFormat_Float32x3,
        .offset = 0,
        .shaderLocation = 0,
    }};
    WGPUVertexBufferLayout shadowmap_vertex_buffer_layout = {
        .arrayStride = sizeof(vec3s),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 1,
        .attributes = shadowmap_vertex_attributes};
    WGPUPrimitiveState shadowmap_primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
        .stripIndexFormat = WGPUIndexFormat_Undefined,
        .frontFace = WGPUFrontFace_CW,
        .cullMode = WGPUCullMode_Back,
    };

    WGPUPushConstantRange shadowmap_constants[] =
        PUSH_CONSTANTS_FOR(ShadowmapPushConstants);
    shaders->lights.shadowmap = create_shader(
        "assets/shaders/shadowmap.wgsl", "shadowmap", &layouts->shadowmap, 1,
        shadowmap_constants, 1, &shadowmap_vertex_buffer_layout, 1,
        shadowmap_targets, 1, NULL, &shadowmap_primitive, resources);
}
