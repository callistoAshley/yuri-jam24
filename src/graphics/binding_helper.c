#include <stdlib.h>
#include "binding_helper.h"
#include "webgpu.h"

static void bind_group_layout_entry_free(size_t index, void *ptr)
{
    (void)index;
    WGPUBindGroupLayoutEntry *entry = ptr;
    if (entry->nextInChain)
    {
        free((void *)entry->nextInChain);
    }
}

static void bind_group_entry_free(size_t index, void *ptr)
{
    (void)index;
    WGPUBindGroupEntry *entry = ptr;
    if (entry->nextInChain)
    {
        free((void *)entry->nextInChain);
    }
}

void bind_group_layout_builder_init(BindGroupLayoutBuilder *builder)
{
    vec_init(&builder->entries, sizeof(WGPUBindGroupLayoutEntry));
}
void bind_group_layout_builder_free(BindGroupLayoutBuilder *builder)
{
    vec_free_with(&builder->entries, bind_group_layout_entry_free);
}

// ---  ---

void bind_group_layout_builder_append(BindGroupLayoutBuilder *builder,
                                      WGPUBindGroupLayoutEntry entry)
{
    entry.binding = builder->entries.len;
    vec_push(&builder->entries, &entry);
}

// ---  ---

WGPUBindGroupLayout bind_group_layout_build(BindGroupLayoutBuilder *builder,
                                            WGPUDevice device,
                                            const char *label)
{
    WGPUBindGroupLayoutDescriptor desc = {
        .label = label,
        .entries = (WGPUBindGroupLayoutEntry *)builder->entries.data,
        .entryCount = builder->entries.len,
    };
    return wgpuDeviceCreateBindGroupLayout(device, &desc);
}

// ---  ---

void bind_group_builder_init(BindGroupBuilder *builder)
{
    vec_init(&builder->entries, sizeof(WGPUBindGroupEntry));
}
void bind_group_builder_free(BindGroupBuilder *builder)
{
    vec_free_with(&builder->entries, bind_group_entry_free);
}

// ---  ---

void bind_group_builder_append(BindGroupBuilder *builder,
                               WGPUBindGroupEntry *entry)
{
    vec_push(&builder->entries, entry);
}
void bind_group_builder_append_buffer(BindGroupBuilder *builder,
                                      WGPUBuffer buffer)
{
    WGPUBindGroupEntry entry = {
        .binding = builder->entries.len,
        .buffer = buffer,
    };
    vec_push(&builder->entries, &entry);
}
void bind_group_builder_append_buffer_with_size(BindGroupBuilder *builder,
                                                WGPUBuffer buffer,
                                                uint64_t size)
{
    WGPUBindGroupEntry entry = {
        .binding = builder->entries.len,
        .buffer = buffer,
        .size = size,
    };
    vec_push(&builder->entries, &entry);
}

void bind_group_builder_append_sampler(BindGroupBuilder *builder,
                                       WGPUSampler sampler)
{
    WGPUBindGroupEntry entry = {
        .binding = builder->entries.len,
        .sampler = sampler,
    };
    vec_push(&builder->entries, &entry);
}

void bind_group_builder_append_sampler_array(BindGroupBuilder *builder,
                                             WGPUSampler *samplers,
                                             uint32_t count)
{
    WGPUBindGroupEntryExtras *extras = malloc(sizeof(WGPUBindGroupEntryExtras));
    extras->samplers = samplers;
    extras->samplerCount = count;
    extras->chain =
        (WGPUChainedStruct){.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras};

    WGPUBindGroupEntry entry = {
        .binding = builder->entries.len,
        .nextInChain = (WGPUChainedStruct *)extras,
    };
    vec_push(&builder->entries, &entry);
}

void bind_group_builder_append_texture_view(BindGroupBuilder *builder,
                                            WGPUTextureView texture_view)
{
    WGPUBindGroupEntry entry = {
        .binding = builder->entries.len,
        .textureView = texture_view,
    };
    vec_push(&builder->entries, &entry);
}

void bind_group_builder_append_texture_view_array(
    BindGroupBuilder *builder, WGPUTextureView *texture_views, uint32_t count)
{
    WGPUBindGroupEntryExtras *extras = malloc(sizeof(WGPUBindGroupEntryExtras));
    extras->textureViews = texture_views;
    extras->textureViewCount = count;
    extras->chain =
        (WGPUChainedStruct){.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras};

    WGPUBindGroupEntry entry = {
        .binding = builder->entries.len,
        .nextInChain = (WGPUChainedStruct *)extras,
    };
    vec_push(&builder->entries, &entry);
}

// ---  ---

WGPUBindGroup bind_group_build(BindGroupBuilder *builder, WGPUDevice device,
                               WGPUBindGroupLayout layout, const char *label)
{
    WGPUBindGroupDescriptor desc = {
        .label = label,
        .layout = layout,
        .entries = (WGPUBindGroupEntry *)builder->entries.data,
        .entryCount = builder->entries.len,
    };
    return wgpuDeviceCreateBindGroup(device, &desc);
}
