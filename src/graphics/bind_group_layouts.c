#include "bind_group_layouts.h"
#include "binding_helper.h"
#include "webgpu.h"
#include "wgpu.h"
#include <stdlib.h>

void build_sprite_layout(BindGroupLayouts *layouts, WGPUResources *resources)
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

    layouts->sprite = bind_group_layout_build(&builder, resources->device,
                                              "Sprite Bind Group Layout");
    bind_group_layout_builder_free(&builder); // free the builder after use
}

void build_light_layout(BindGroupLayouts *layouts, WGPUResources *resources)
{
    BindGroupLayoutBuilder builder;
    bind_group_layout_builder_init(&builder);

    WGPUTextureBindingLayout texture_layout = {
        .sampleType = WGPUTextureSampleType_UnfilterableFloat,
        .viewDimension = WGPUTextureViewDimension_2D,
    };

    WGPUBindGroupLayoutEntry entry = {
        .texture = texture_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);

    WGPUSamplerBindingLayout sampler_layout = {
        .type = WGPUSamplerBindingType_NonFiltering,
    };
    entry = (WGPUBindGroupLayoutEntry){
        .sampler = sampler_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);

    layouts->lighting = bind_group_layout_build(&builder, resources->device,
                                                "Lighting Bind Group Layout");

    bind_group_layout_builder_free(&builder);
}

void build_tilemap_layout(BindGroupLayouts *layouts, WGPUResources *resources)
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
        .visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex,
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

    layouts->tilemap = bind_group_layout_build(&builder, resources->device,
                                               "Tilemap Bind Group Layout");
    bind_group_layout_builder_free(&builder); // free the builder after use
}

void build_hdr_tonemap_layout(BindGroupLayouts *layouts,
                              WGPUResources *resources)
{
    BindGroupLayoutBuilder builder;
    bind_group_layout_builder_init(&builder);

    WGPUTextureBindingLayout texture_layout = {
        .sampleType = WGPUTextureSampleType_Float,
        .viewDimension = WGPUTextureViewDimension_2D,
    };
    WGPUBindGroupLayoutEntry entry = {
        .texture = texture_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);

    WGPUSamplerBindingLayout sampler_layout = {
        .type = WGPUSamplerBindingType_NonFiltering,
    };
    entry = (WGPUBindGroupLayoutEntry){
        .sampler = sampler_layout,
        .visibility = WGPUShaderStage_Fragment,
    };
    bind_group_layout_builder_append(&builder, entry);

    layouts->hdr_tonemap = bind_group_layout_build(
        &builder, resources->device, "HDR Tonemap Bind Group Layout");
    bind_group_layout_builder_free(&builder);
}

void bind_group_layouts_init(BindGroupLayouts *layouts,
                             WGPUResources *resources)
{
    build_sprite_layout(layouts, resources);
    build_light_layout(layouts, resources);
    build_tilemap_layout(layouts, resources);
    build_hdr_tonemap_layout(layouts, resources);
}

void bind_group_layouts_free(BindGroupLayouts *layouts)
{
    wgpuBindGroupLayoutRelease(layouts->sprite);
    wgpuBindGroupLayoutRelease(layouts->lighting);
    wgpuBindGroupLayoutRelease(layouts->tilemap);
    wgpuBindGroupLayoutRelease(layouts->hdr_tonemap);
}
