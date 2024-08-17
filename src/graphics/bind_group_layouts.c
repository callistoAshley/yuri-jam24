#include "bind_group_layouts.h"
#include "binding_helper.h"
#include "webgpu.h"
#include "wgpu.h"

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

    WGPUBindGroupLayoutEntryExtras *extras =
        malloc(sizeof(WGPUBindGroupLayoutEntryExtras));
    *extras = (WGPUBindGroupLayoutEntryExtras){
        .chain = {.sType = (WGPUSType)WGPUSType_BindGroupLayoutEntryExtras},
        .count = 2048,
    };
    WGPUTextureBindingLayout texture_layout = {
        .sampleType = WGPUTextureSampleType_Float,
        .viewDimension = WGPUTextureViewDimension_2D,
    };
    entry = (WGPUBindGroupLayoutEntry){
        .nextInChain = (WGPUChainedStruct *)extras,
        .texture = texture_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);

    WGPUSamplerBindingLayout sampler_layout = {
        .type = WGPUSamplerBindingType_Filtering,
    };
    entry = (WGPUBindGroupLayoutEntry){
        .sampler = sampler_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);

    layouts->object = bind_group_layout_build(&builder, resources->device,
                                              "Basic Bind Group Layout");
    bind_group_layout_builder_free(&builder); // free the builder after use

    bind_group_layout_builder_init(&builder);

    texture_layout.sampleType = WGPUTextureSampleType_UnfilterableFloat;
    entry = (WGPUBindGroupLayoutEntry){
        .texture = texture_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);
    bind_group_layout_builder_append(&builder, entry);

    sampler_layout.type = WGPUSamplerBindingType_NonFiltering;
    entry = (WGPUBindGroupLayoutEntry){
        .sampler = sampler_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);

    layouts->lighting = bind_group_layout_build(&builder, resources->device,
                                                "Lighting Bind Group Layout");

    bind_group_layout_builder_free(&builder);
}
