#include "shaders.h"
#include "core_types.h"
#include "graphics/bind_group_layouts.h"
#include "utility/macros.h"
#include "webgpu.h"

#include <wgpu.h>
#include <stdio.h>

void read_entire_file(const char *path, char **out, long *len)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        FATAL("ERROR: Could not open file %s\n", path);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);

    *out = buffer;
    *len = length;
}

void shaders_init(Shaders *shaders, BindGroupLayouts *layouts,
                  WGPUResources *resources)
{
    char *buf;
    long buf_len;

    read_entire_file("assets/shaders/basic.wgsl", &buf, &buf_len);

    // this uses null-terminated strings, but read_entire_file returns something
    // null-terminated anyway, so it's fine (really wish this used a
    // length-based thing instead!!)
    WGPUShaderModuleWGSLDescriptor wgsl_module_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor shader_module_descriptor = {
        .label = "basic",
        .nextInChain = (WGPUChainedStruct *)&wgsl_module_descriptor,
    };

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(
        resources->device, &shader_module_descriptor);

    typedef struct
    {
        mat4s camera;
        u32 transform_index;
        u32 texture_index;
    } PushConstants;
    WGPUPushConstantRange push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(PushConstants),
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
        .bindGroupLayouts = &layouts->basic,
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

    WGPUVertexState vertex_state = {.module = shader_module,
                                    .entryPoint = "vs_main",
                                    .buffers = &vertex_buffer_layout,
                                    .bufferCount = 1};

    WGPUColorTargetState color_targets[] = {(WGPUColorTargetState){
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
    }};
    WGPUFragmentState fragment_state = {.module = shader_module,
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
        .label = "basic",
        .layout = layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };
    shaders->basic =
        wgpuDeviceCreateRenderPipeline(resources->device, &descriptor);
    PTR_ERRCHK(shaders->basic, "failed to create render pipeline");

    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(shader_module);
}
