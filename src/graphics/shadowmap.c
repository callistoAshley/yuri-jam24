#include "shadowmap.h"
#include "utility/macros.h"

void shadowmap_init(ShadowMap *shadowmap, WGPUResources *wgpu)
{

    WGPUExtent3D extents = {
        .width = INTERNAL_SCREEN_WIDTH * SHADOWMAP_WIDTH,
        .height = INTERNAL_SCREEN_HEIGHT * SHADOWMAP_HEIGHT,
        .depthOrArrayLayers = 1,
    };
    WGPUTextureDescriptor desc = {
        .label = "shadow mask texture",
        .size = extents,
        .dimension = WGPUTextureDimension_2D,
        .format = WGPUTextureFormat_R8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .usage =
            WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
    };
    shadowmap->texture = wgpuDeviceCreateTexture(wgpu->device, &desc);
    shadowmap->texture_view = wgpuTextureCreateView(shadowmap->texture, NULL);

    shadowmap->next = 0;
    shadowmap->filled_to = 0;
}

void shadowmap_free(ShadowMap *shadowmap)
{
    wgpuTextureViewRelease(shadowmap->texture_view);
    wgpuTextureRelease(shadowmap->texture);
}

ShadowMapEntry shadowmap_add(ShadowMap *shadowmap, vec2s position)
{
    ShadowMapEntry entry = shadowmap->next;

    if (shadowmap->filled_to == MAX_SHADOW_COUNT)
        FATAL("Too many lights that cast shadows!!");

    if (shadowmap->next == shadowmap->filled_to)
    {
        shadowmap->entries[entry].position = position;
        shadowmap->filled_to++;
        shadowmap->next++;
    }
    else
    {
        ShadowMapEntryData *entry_data = &shadowmap->entries[shadowmap->next];
        entry_data->position = position;
        shadowmap->next = entry_data->next.entry;
    }

    return entry;
}

void shadowmap_remove(ShadowMap *shadowmap, ShadowMapEntry entry)
{
    ShadowMapEntryData *data = &shadowmap->entries[entry];
    data->next.is_free = SHADOWMAP_ENTRY_FREE;
    data->next.entry = shadowmap->next;
    shadowmap->next = entry;
}

void shadowmap_iter_init(ShadowMap *shadowmap, ShadowMapIter *iter)
{
    iter->shadowmap = shadowmap;
    iter->current_entry = 0;
}

bool shadowmap_iter_next(ShadowMapIter *iter, vec2s *position)
{
    ShadowMap *shadowmap = iter->shadowmap;
    while (iter->current_entry < shadowmap->filled_to)
    {
        ShadowMapEntryData *data = &shadowmap->entries[iter->current_entry];
        iter->current_entry++;
        if (data->next.is_free != SHADOWMAP_ENTRY_FREE)
        {
            *position = data->position;
            return true;
        }
    }

    return false;
}
