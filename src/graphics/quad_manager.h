#pragma once

#include <wgpu.h>
#include "graphics/wgpu_resources.h"
#include "core_types.h"
#include "sensible_nums.h"
#include "utility/hashset.h"
#include "utility/vec.h"

typedef struct
{
    WGPUBuffer buffer;
    vec entries; // either occupied, or an index to the next free entry
    HashSet dirty_entries;
    u32 next;
} QuadManager;

#define QUAD_ENTRY_FREE UINT32_MAX
#define QUAD_ENTRY_TO_VERTEX_INDEX(entry) ((entry) * VERTICES_PER_QUAD)
typedef u32 QuadEntry;

void quad_manager_init(QuadManager *manager, WGPUResources *resources);
void quad_manager_free(QuadManager *manager);

QuadEntry quad_manager_add(QuadManager *manager, Quad quad);
void quad_manager_remove(QuadManager *manager, QuadEntry entry);
void quad_manager_update(QuadManager *manager, QuadEntry entry, Quad quad);
// WARNING: this is not slow, but it's not very fast either! store quads if you
// plan to update them!
Quad quad_manager_get(QuadManager *manager, QuadEntry entry);

// call before using for rendering
void quad_manager_upload_dirty(QuadManager *manager, WGPUResources *resources);
