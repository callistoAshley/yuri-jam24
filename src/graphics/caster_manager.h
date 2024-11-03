#pragma once

#include <wgpu.h>
#include "graphics/transform_manager.h"
#include "graphics/wgpu_resources.h"
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
} CasterEntry;

typedef struct
{
    vec casters; // vec<vec3s>
    vec entries; // a list of CasterEntry
    WGPUBuffer buffer;
    bool dirty;
} CasterManager;

typedef struct
{
    TransformEntry transform;
    vec2s offset;
    CasterEntry *caster;
    f32 radius;
    u32 cell;
} ShadowCaster;

void caster_manager_init(CasterManager *manager, WGPUResources *resources);
void caster_manager_free(CasterManager *manager);

// returns a reference to the caster at the given path
CasterEntry *caster_manager_load(CasterManager *manager, const char *path);
// WARNING: this will not check if the path already exists!
CasterEntry *caster_manager_register(CasterManager *manager, Cell *cells,
                                     u32 cell_count);
void caster_manager_write_dirty(CasterManager *manager,
                                WGPUResources *resources);

// temporary hack to clear the manager because we don't support unloading yet
void caster_manager_clear(CasterManager *manager);
