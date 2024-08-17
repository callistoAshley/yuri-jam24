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

    read_entire_file("assets/shaders/object.wgsl", &buf, &buf_len);

    // this uses null-terminated strings, but read_entire_file returns something
    // null-terminated anyway, so it's fine (really wish this used a
    // length-based thing instead!!)
    WGPUShaderModuleWGSLDescriptor object_wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor object_module_descriptor = {
        .label = "object",
        .nextInChain = (WGPUChainedStruct *)&object_wgsl_descriptor,
    };

    WGPUShaderModule object_module = wgpuDeviceCreateShaderModule(
        resources->device, &object_module_descriptor);

    WGPUPushConstantRange object_push_constant_ranges[] = {
        (WGPUPushConstantRange){
            .stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .start = 0,
            .end = sizeof(ObjectPushConstants),
        },
    };
    WGPUPipelineLayoutExtras object_extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras},
        .pushConstantRanges = object_push_constant_ranges,
        .pushConstantRangeCount = 1,
    };
    WGPUPipelineLayoutDescriptor object_layout_descriptor = {
        .nextInChain = (WGPUChainedStruct *)&object_extras,
        .label = "basic",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->object,
    };
    WGPUPipelineLayout object_layout = wgpuDeviceCreatePipelineLayout(
        resources->device, &object_layout_descriptor);

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

    WGPUVertexState vertex_state = {.module = object_module,
                                    .entryPoint = "vs_main",
                                    .buffers = &vertex_buffer_layout,
                                    .bufferCount = 1};

    WGPUColorTargetState object_color_targets[] = {
        // color
        (WGPUColorTargetState){
            .format = WGPUTextureFormat_RGBA8Unorm,
            .writeMask = WGPUColorWriteMask_All,
        },
        // normal
        (WGPUColorTargetState){
            .format = WGPUTextureFormat_RGBA32Float,
            .writeMask = WGPUColorWriteMask_All,
        }};
    WGPUFragmentState fragment_state = {.module = object_module,
                                        .entryPoint = "fs_main",
                                        .targetCount = 2,
                                        .targets = object_color_targets};

    WGPUPrimitiveState primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
    };
    WGPUMultisampleState multisample = {
        .count = 1,
        .mask = 0xFFFFFFFF,
    };

    WGPURenderPipelineDescriptor object_descriptor = {
        .label = "object",
        .layout = object_layout,
        .vertex = vertex_state,
        .fragment = &fragment_state,
        .primitive = primitive,
        .multisample = multisample,
    };
    shaders->object =
        wgpuDeviceCreateRenderPipeline(resources->device, &object_descriptor);
    PTR_ERRCHK(shaders->object, "failed to create render pipeline");

    free(buf);
    wgpuPipelineLayoutRelease(object_layout);
    wgpuShaderModuleRelease(object_module);

    read_entire_file("assets/shaders/light.wgsl", &buf, &buf_len);

    WGPUShaderModuleWGSLDescriptor lighting_wgsl_descriptor = {
        .chain = {.sType = WGPUSType_ShaderModuleWGSLDescriptor},
        .code = buf,
    };
    WGPUShaderModuleDescriptor lighting_module_descriptor = {
        .label = "lighting",
        .nextInChain = (WGPUChainedStruct *)&lighting_wgsl_descriptor,
    };

    WGPUShaderModule lighting_module = wgpuDeviceCreateShaderModule(
        resources->device, &lighting_module_descriptor);

    WGPUPipelineLayoutDescriptor layout_descriptor_lighting = {
        .label = "lighting",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &layouts->lighting,
    };

    WGPUPipelineLayout layout_lighting = wgpuDeviceCreatePipelineLayout(
        resources->device, &layout_descriptor_lighting);

    WGPUVertexState vertex_state_lighting = {
        .module = lighting_module,
        .entryPoint = "vs_main",
        .buffers = &vertex_buffer_layout,
        .bufferCount = 1,
    };

    WGPUColorTargetState lighting_color_targets[] = {(WGPUColorTargetState){
        .format = resources->surface_config.format,
        .writeMask = WGPUColorWriteMask_All,
    }};

    WGPUFragmentState fragment_state_lighting = {
        .module = lighting_module,
        .entryPoint = "fs_main",
        .targetCount = 1,
        .targets = lighting_color_targets,
    };

    WGPURenderPipelineDescriptor lighting_descriptor = {
        .label = "lighting",
        .layout = layout_lighting,
        .vertex = vertex_state_lighting,
        .fragment = &fragment_state_lighting,
        .primitive = primitive,
        .multisample = multisample,
    };

    shaders->lighting =
        wgpuDeviceCreateRenderPipeline(resources->device, &lighting_descriptor);

    free(buf);
    wgpuPipelineLayoutRelease(layout_lighting);
    wgpuShaderModuleRelease(lighting_module);
}
