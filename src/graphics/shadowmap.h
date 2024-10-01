#pragma once

#include <cglm/struct.h>
#include "graphics/wgpu_resources.h"
#include "sensible_nums.h"
#include "utility/common_defines.h"

// 16 x INTERNAL_SCREEN_WIDTH/INTERNAL_SCREEN_HEIGHT
#define SHADOWMAP_WIDTH 16
#define SHADOWMAP_HEIGHT 16
#define MAX_SHADOW_COUNT SHADOWMAP_WIDTH *SHADOWMAP_HEIGHT

#define SHADOWMAP_ENTRY_FREE UINT32_MAX

typedef u32 ShadowMapEntry;

typedef union
{
    vec2s position;
    struct
    {
        u32 is_free;
        ShadowMapEntry entry;
    } next;
} ShadowMapEntryData;

typedef struct
{
    WGPUTexture texture;
    WGPUTextureView texture_view;

    u32 next, filled_to;
    ShadowMapEntryData entries[MAX_SHADOW_COUNT];
} ShadowMap;

typedef struct
{
    ShadowMap *shadowmap;
    ShadowMapEntry current_entry;
} ShadowMapIter;

#define SHADOWMAP_ENTRY_POS_OFFSET(entry)                                      \
    ((vec2s){.x = (f32)(entry % SHADOWMAP_WIDTH) * INTERNAL_SCREEN_WIDTH,      \
             .y = (f32)(entry / SHADOWMAP_WIDTH) * INTERNAL_SCREEN_HEIGHT})

void shadowmap_init(ShadowMap *shadowmap, WGPUResources *wgpu);

ShadowMapEntry shadowmap_add(ShadowMap *shadowmap, vec2s position);
void shadowmap_remove(ShadowMap *shadowmap, ShadowMapEntry entry);

void shadowmap_iter_init(ShadowMap *shadowmap, ShadowMapIter *iter);
bool shadowmap_iter_next(ShadowMapIter *iter, vec2s *position);
