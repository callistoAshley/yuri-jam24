#pragma once

#include <cglm/struct.h>
#include "graphics/wgpu_resources.h"
#include "sensible_nums.h"
#include "utility/common_defines.h"

// 8 x GAME_VIEW_WIDTH/GAME_VIEW_HEIGHT
#define SHADOWMAP_WIDTH 8
#define SHADOWMAP_HEIGHT 8
#define MAX_SHADOW_COUNT (SHADOWMAP_WIDTH * SHADOWMAP_HEIGHT)

#define SHADOWMAP_ENTRY_FREE UINT32_MAX

typedef u32 ShadowMapEntry;

typedef union
{
    struct
    {
        vec2s position;
        f32 radius;
    } inner;
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
    ((vec2s){.x = (u32)((entry) % SHADOWMAP_WIDTH) * GAME_VIEW_WIDTH,          \
             .y = (u32)((entry) / SHADOWMAP_WIDTH) * GAME_VIEW_HEIGHT})

void shadowmap_init(ShadowMap *shadowmap, WGPUResources *wgpu);
void shadowmap_free(ShadowMap *shadowmap);

// radius is used for clipping, -1 = do not clip
ShadowMapEntry shadowmap_add(ShadowMap *shadowmap, vec2s position, f32 radius);
void shadowmap_remove(ShadowMap *shadowmap, ShadowMapEntry entry);

void shadowmap_iter_init(ShadowMap *shadowmap, ShadowMapIter *iter);
bool shadowmap_iter_next(ShadowMapIter *iter, vec2s *position, f32 *radius);
