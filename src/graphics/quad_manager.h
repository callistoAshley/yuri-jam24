#pragma once

#include <wgpu.h>
#include "graphics/wgpu_resources.h"
#include "core_types.h"
#include "sensible_nums.h"
#include "utility/vec.h"

typedef struct
{
    WGPUBuffer buffer;
    vec entries; // either occupied, or an index to the next free entry
    bool dirty;
    u32 next;
} QuadManager;

#define ENTRY_FREE UINT32_MAX
typedef u32 QuadEntry;

void quad_manager_init(QuadManager *manager, WGPUResources *resources);
void quad_manager_free(QuadManager *manager);

QuadEntry quad_manager_add(QuadManager *manager, Quad quad);
void quad_manager_remove(QuadManager *manager, QuadEntry entry);
void quad_manager_update(QuadManager *manager, QuadEntry entry, Quad quad);

// call before using for rendering
void quad_manager_upload_dirty(QuadManager *manager, WGPUResources *resources);
