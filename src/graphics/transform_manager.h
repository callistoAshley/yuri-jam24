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
    bool dirty;  // a list of dirty entries vec<TransformEntry>
    u32 next;
} TransformManager;

#define ENTRY_FREE UINT32_MAX
typedef u32 TransformEntry;

void transform_manager_init(TransformManager *manager,
                            WGPUResources *resources);
void transform_manager_free(TransformManager *manager);

TransformEntry transform_manager_add(TransformManager *manager,
                                     Transform transform);
void transform_manager_remove(TransformManager *manager, TransformEntry entry);
void transform_manager_update(TransformManager *manager, TransformEntry entry,
                              Transform transform);

// call before using for rendering
void transform_manager_upload_dirty(TransformManager *manager,
                                    WGPUResources *resources);
