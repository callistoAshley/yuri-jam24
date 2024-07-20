#include "bind_group_layouts.h"
#include "binding_helper.h"
#include "webgpu.h"

void bing_group_layouts_init(BindGroupLayouts *layouts,
                             WGPUResources *resources)
{
    BindGroupLayoutBuilder builder;
    bind_group_layout_builder_init(&builder);

    WGPUBufferBindingLayout buffer_layout = {
        .type = WGPUBufferBindingType_ReadOnlyStorage,
    };
    WGPUBindGroupLayoutEntry entry = {
        .buffer = buffer_layout,
        .visibility = WGPUShaderStage_Vertex,
    };
    bind_group_layout_builder_append(&builder, entry);

    layouts->basic = bind_group_layout_build(&builder, resources->device,
                                             "Basic Bind Group Layout");
    bind_group_layout_builder_free(&builder); // free the builder after use
}
