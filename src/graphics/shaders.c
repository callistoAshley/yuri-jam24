#include "shaders.h"
#include "core_types.h"
#include "graphics/bind_group_layouts.h"
#include "sensible_nums.h"
#include "utility/macros.h"
#include "utility/files.h"
#include "webgpu.h"

#include <wgpu.h>
#include <stdio.h>

void create_sprite_shader(Shaders *shaders, BindGroupLayouts *layouts,
                          WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/sprite.wgsl", &buf, &buf_len);

    // this uses null-terminated strings, but read_entire_file returns something
    // null-terminated anyway, so it's fine (really wish this used a
    // length-based thing instead!!)
    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "sprite",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(SpritePushConstants),
        },
    };
    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = 1,
    };
    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = "basic",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->sprite,
    };
    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexAttribute vertex_attributes[] = {
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        },
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = 2 * sizeof(float),
            .shaderLocation = 1,
        }};
    WGPUVertexBufferLayout vertex_buffer_layout = {
        .arrayStride = sizeof(Vertex),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 2,
        .attributes = vertex_attributes};

    WGPUVertexState vertex_state = {.module = module,
                                    .entryPoint = "vs_main",
                                    .buffers = &vertex_buffer_layout,
                                    .bufferCount = 1};

    WGPUColorTargetState color_targets[] = {
        // color
        (WGPUColorTargetState){
            .format = WGPUTextureFormat_RGBA8Unorm,
            .writeMask = WGPUColorWriteMask_All,
        }};
    WGPUFragmentState fragment_state = {.module = module,
                                        .entryPoint = "fs_main",
                                        .targetCount = 1,
                                        .targets = color_targets};

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };
    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "sprite",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };
    shaders->defferred.sprite =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);
    PTR_ERRCHK(shaders->defferred.sprite, "failed to create render pipeline");

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);
}

void create_point_light_shader(Shaders *shaders, BindGroupLayouts *layouts,
                               WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/point_light.wgsl", &buf, &buf_len);

    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "point_light",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(PointLightPushConstants),
        },
    };
    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = 1,
    };

    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = "point_light",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->lighting,
    };

    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexState vertex_state = {
        .module = module,
        .entryPoint = "vs_main",
    };

    // perform additive blending
    WGPUBlendComponent color = {
        .srcFactor = WGPUBlendFactor_One,
        .dstFactor = WGPUBlendFactor_One,
        .operation = WGPUBlendOperation_Add,
    };
    WGPUBlendComponent alpha = {
        .srcFactor = WGPUBlendFactor_One,
        .dstFactor = WGPUBlendFactor_One,
        .operation = WGPUBlendOperation_Add,
    };
    WGPUBlendState blend = {
        .color = color,
        .alpha = alpha,
    };

    WGPUColorTargetState color_targets[] = {(WGPUColorTargetState){
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
        .blend = &blend,
    }};

    WGPUFragmentState fragment_state = {
        .module = module,
        .entryPoint = "fs_main",
        .targetCount = 1,
        .targets = color_targets,
    };

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };
    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "point_light",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };

    shaders->lights.point =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);
}

void create_directional_light_shader(Shaders *shaders,
                                     BindGroupLayouts *layouts,
                                     WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/direct_light.wgsl", &buf, &buf_len);

    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "direct_light",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(DirectLightPushConstants),
        },
    };
    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = 1,
    };

    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = "direct_light",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->lighting,
    };

    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexState vertex_state = {
        .module = module,
        .entryPoint = "vs_main",
    };

    // perform additive blending
    WGPUBlendComponent color = {
        .srcFactor = WGPUBlendFactor_One,
        .dstFactor = WGPUBlendFactor_One,
        .operation = WGPUBlendOperation_Add,
    };
    WGPUBlendComponent alpha = {
        .srcFactor = WGPUBlendFactor_One,
        .dstFactor = WGPUBlendFactor_One,
        .operation = WGPUBlendOperation_Add,
    };
    WGPUBlendState blend = {
        .color = color,
        .alpha = alpha,
    };

    WGPUColorTargetState color_targets[] = {(WGPUColorTargetState){
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
        .blend = &blend,
    }};

    WGPUFragmentState fragment_state = {
        .module = module,
        .entryPoint = "fs_main",
        .targetCount = 1,
        .targets = color_targets,
    };

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };
    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "direct_light",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };

    shaders->lights.direct =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);
}

void create_tilemap_shader(Shaders *shaders, BindGroupLayouts *layouts,
                           WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/tilemap.wgsl", &buf, &buf_len);

    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };

    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "tilemap",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(TilemapPushConstants),
        },
    };
    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = 1,
    };
    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = "tilemap",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->tilemap,
    };

    WGPUPipelineLayout tilemap =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexAttribute vertex_attributes[] = {(WGPUVertexAttribute){
        .format = WGPUVertexFormat_Sint32,
        .offset = 0,
        .shaderLocation = 0,
    }};
    WGPUVertexBufferLayout vertex_buffer_layout = {
        .arrayStride = sizeof(u32),
        .stepMode = WGPUVertexStepMode_Instance,
        .attributeCount = 1,
        .attributes = vertex_attributes,
    };

    WGPUVertexState vertex_state = {
        .module = module,
        .entryPoint = "vs_main",
        .buffers = &vertex_buffer_layout,
        .bufferCount = 1,
    };

    WGPUColorTargetState color_targets[] = {
        // color
        (WGPUColorTargetState){
            .format = WGPUTextureFormat_RGBA8Unorm,
            .writeMask = WGPUColorWriteMask_All,
        }};
    WGPUFragmentState fragment_state = {.module = module,
                                        .entryPoint = "fs_main",
                                        .targetCount = 1,
                                        .targets = color_targets};

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };

    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "tilemap",
        .layout = tilemap,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };

    shaders->defferred.tilemap =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);

    free(buf);
    wgpuPipelineLayoutRelease(tilemap);
    wgpuShaderModuleRelease(module);
}

void create_b2d_circle_shader(Shaders *shaders, WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/b2d_circle.wgsl", &buf, &buf_len);

    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };

    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "b2d_circle",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(B2DCirclePushConstants),
        },
    };
    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = 1,
    };
    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = "b2d_circle",
    };

    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexAttribute vertex_attributes[] = {
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        },
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = 2 * sizeof(float),
            .shaderLocation = 1,
        }};
    WGPUVertexBufferLayout vertex_buffer_layout = {
        .arrayStride = sizeof(Vertex),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 2,
        .attributes = vertex_attributes};

    WGPUVertexState vertex_state = {
        .module = module,
        .entryPoint = "vs_main",
        .buffers = &vertex_buffer_layout,
        .bufferCount = 1,
    };

    WGPUBlendState blend = {
        .color =
            {
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                .operation = WGPUBlendOperation_Add,
            },
        .alpha =
            {
                .srcFactor = WGPUBlendFactor_One,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                .operation = WGPUBlendOperation_Add,
            },
    };

    WGPUColorTargetState color_targets[] = {
        // color
        (WGPUColorTargetState){
            .format = resources->surface_config.format,
            .writeMask = WGPUColorWriteMask_All,
            .blend = &blend,
        },
    };
    WGPUFragmentState fragment_state = {.module = module,
                                        .entryPoint = "fs_main",
                                        .targetCount = 1,
                                        .targets = color_targets};

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };

    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "b2d_circle",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };

    shaders->box2d_debug.circle =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);
}

void create_b2d_polygon_shader(Shaders *shaders, WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/b2d_polygon.wgsl", &buf, &buf_len);

    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };

    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "b2d_polygon",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(B2DDrawPolygonPushConstants),
        },
    };
    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = 1,
    };
    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = "b2d_polygon",
    };

    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexAttribute vertex_attributes[] = {(WGPUVertexAttribute){
        .format = WGPUVertexFormat_Float32x2,
        .offset = 0,
        .shaderLocation = 0,
    }};
    WGPUVertexBufferLayout vertex_buffer_layout = {
        .arrayStride = sizeof(f32) * 2,
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 1,
        .attributes = vertex_attributes};

    WGPUVertexState vertex_state = {
        .module = module,
        .entryPoint = "vs_main",
        .buffers = &vertex_buffer_layout,
        .bufferCount = 1,
    };

    WGPUBlendState blend = {
        .color =
            {
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                .operation = WGPUBlendOperation_Add,
            },
        .alpha =
            {
                .srcFactor = WGPUBlendFactor_One,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                .operation = WGPUBlendOperation_Add,
            },
    };

    WGPUColorTargetState color_targets[] = {
        // color
        (WGPUColorTargetState){
            .format = resources->surface_config.format,
            .writeMask = WGPUColorWriteMask_All,
            .blend = &blend,
        },
    };
    WGPUFragmentState fragment_state = {.module = module,
                                        .entryPoint = "fs_main",
                                        .targetCount = 1,
                                        .targets = color_targets};

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };

    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "b2d_polygon",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };

    shaders->box2d_debug.polygon =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);
}

void create_ui_sprite_shader(Shaders *shaders, BindGroupLayouts *layouts,
                             WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/ui_sprite.wgsl", &buf, &buf_len);

    // this uses null-terminated strings, but read_entire_file returns something
    // null-terminated anyway, so it's fine (really wish this used a
    // length-based thing instead!!)
    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "ui_sprite",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(SpritePushConstants),
        },
    };
    WGPUPipelineLayoutExtras extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = push_constant_ranges,
        .pushConstantRangeCount = 1,
    };
    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&extras,
        .label = "basic",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->sprite,
    };
    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexAttribute vertex_attributes[] = {
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        },
        (WGPUVertexAttribute){
            .format = WGPUVertexFormat_Float32x2,
            .offset = 2 * sizeof(float),
            .shaderLocation = 1,
        }};
    WGPUVertexBufferLayout vertex_buffer_layout = {
        .arrayStride = sizeof(Vertex),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 2,
        .attributes = vertex_attributes};

    WGPUVertexState vertex_state = {.module = module,
                                    .entryPoint = "vs_main",
                                    .buffers = &vertex_buffer_layout,
                                    .bufferCount = 1};

    WGPUColorTargetState color_targets[] = {
        (WGPUColorTargetState){
            .format = resources->surface_config.format,
            .writeMask = WGPUColorWriteMask_All,
        },
    };
    WGPUFragmentState fragment_state = {.module = module,
                                        .entryPoint = "fs_main",
                                        .targetCount = 1,
                                        .targets = color_targets};

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };
    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "ui_sprite",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };
    shaders->forward.ui_sprite =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);
    PTR_ERRCHK(shaders->forward.ui_sprite, "failed to create render pipeline");

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);
}

// this is extremely simple
// use this as a reference for creating other shaders if you want
void create_screen_blit_shader(Shaders *shaders, BindGroupLayouts *layouts,
                               WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/screen_blit.wgsl", &buf, &buf_len);

    // this uses null-terminated strings, but read_entire_file returns something
    // null-terminated anyway, so it's fine (really wish this used a
    // length-based thing instead!!)
    WGPUShaderModuleWGSLDescriptor wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor module_descriptor = {
        .label = "screen_blit",
        .nextInChain = (WGPUChainedStruct *)&wgsl_descriptor,
    };

    WGPUShaderModule module =
        wgpuDeviceCreateShaderModule(resources->device, &module_descriptor);

    // this shader is relatively simple (just the one bind group) so we don't
    // need any fancy vertex layouts or push constants
    WGPUPipelineLayoutDescriptor layout_descriptor = {
        .label = "screen_blit",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->screen_blit,
    };

    WGPUPipelineLayout layout =
        wgpuDeviceCreatePipelineLayout(resources->device, &layout_descriptor);

    WGPUVertexState vertex_state = {
        .module = module,
        .entryPoint = "vs_main",
    };

    WGPUColorTargetState color_targets[] = {{
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
    }};

    WGPUFragmentState fragment_state = {.module = module,
                                        .entryPoint = "fs_main",
                                        .targetCount = 1,
                                        .targets = color_targets};

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };

    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor descriptor = {
        .label = "screen_blit",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };

    shaders->forward.screen_blit =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);

    free(buf);
    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(module);
}

void shaders_init(Shaders *shaders, BindGroupLayouts *layouts,
                  WGPUResources *resources)
{
    create_sprite_shader(shaders, layouts, resources);
    create_tilemap_shader(shaders, layouts, resources);

    create_ui_sprite_shader(shaders, layouts, resources);

    create_point_light_shader(shaders, layouts, resources);
    create_directional_light_shader(shaders, layouts, resources);

    create_screen_blit_shader(shaders, layouts, resources);

    create_b2d_circle_shader(shaders, resources);
    create_b2d_polygon_shader(shaders, resources);
}
