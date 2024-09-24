#pragma once

#include <wgpu.h>
#include "graphics/wgpu_resources.h"
#include "core_types.h"
#include "parsers/shdw.h"
#include "sensible_nums.h"
#include "utility/vec.h"

typedef struct
{
    u32 start;
    u32 end;
} CasterCell;

// TODO support unloading casters
// we would need to defragment the buffer
typedef struct
{
    u32 cell_count;
    CasterCell *cells;
    const char *path; // DO NOT MODIFY!
} CasterEntry;

typedef struct
{
    vec casters; // vec<vec3s>
    vec entries; // a list of CasterEntry
    WGPUBuffer buffer;
    bool dirty;
} CasterManager;

void caster_manager_init(CasterManager *manager, WGPUResources *resources);
void caster_manager_free(CasterManager *manager);

// returns a reference to the caster at the given path
// O(n) time complexity. Will return the caster if it already exists
// if the caster does not exist, it will be loaded
// NOTE: the path IS copied!
CasterEntry *caster_manager_load(CasterManager *manager, const char *path);
// WARNING: this will not check if the path already exists!
CasterEntry *caster_manager_register(CasterManager *manager, const char *path,
                                     Cell *cells, u32 cell_count);
void caster_manager_write_dirty(CasterManager *manager,
                                WGPUResources *resources);

// temporary hack to clear the manager because we don't support unloading yet
void caster_manager_clear(CasterManager *manager);
