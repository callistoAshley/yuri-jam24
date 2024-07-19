#pragma once

#include <wgpu.h>
#include "utility/vec.h"

typedef struct
{
    vec entries;
} BindGroupBuilder;

typedef struct
{
    vec entries;
} BindGroupLayoutBuilder;

void bind_group_layout_builder_init(BindGroupLayoutBuilder *builder);
void bind_group_layout_builder_free(BindGroupLayoutBuilder *builder);

// NOTE: This function TAKES the entry! If it has a chained pointer, it will be
// freed when the builder is freed!
// FIXME make this nicer to use, like regular builder is
void bind_group_layout_builder_append(BindGroupLayoutBuilder *builder,
                                      WGPUBindGroupLayoutEntry entry);

WGPUBindGroupLayout bind_group_layout_build(BindGroupLayoutBuilder *builder,
                                            WGPUDevice device,
                                            const char *label);

void bind_group_builder_init(BindGroupBuilder *builder);
void bind_group_builder_free(BindGroupBuilder *builder);

void bind_group_builder_append(BindGroupBuilder *builder,
                               WGPUBindGroupEntry *entry);
void bind_group_builder_append_buffer(BindGroupBuilder *builder,
                                      WGPUBuffer buffer);
void bind_group_builder_append_buffer_with_size(BindGroupBuilder *builder,
                                                WGPUBuffer buffer,
                                                uint64_t size);

void bind_group_builder_append_sampler(BindGroupBuilder *builder,
                                       WGPUSampler sampler);
void bind_group_builder_append_sampler_array(BindGroupBuilder *builder,
                                             WGPUSampler *samplers,
                                             uint32_t count);

void bind_group_builder_append_texture_view(BindGroupBuilder *builder,
                                            WGPUTextureView texture_view);
void bind_group_builder_append_texture_view_array(
    BindGroupBuilder *builder, WGPUTextureView *texture_views, uint32_t count);

WGPUBindGroup bind_group_build(BindGroupBuilder *builder, WGPUDevice device,
                               WGPUBindGroupLayout layout, const char *label);
