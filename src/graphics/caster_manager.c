#include "caster_manager.h"
#include "parsers/shdw.h"
#include "utility/macros.h"

#define INITIAL_BUFFER_CAP 256
#define INITIAL_BUFFER_SIZE sizeof(vec2s) * INITIAL_BUFFER_CAP

void caster_manager_init(CasterManager *manager, WGPUResources *resources)
{
    vec_init(&manager->casters, sizeof(vec2s));
    // need to store pointers to make sure they have a stable address
    vec_init(&manager->entries, sizeof(CasterEntry *));
    WGPUBufferDescriptor buffer_desc = {
        .size = INITIAL_BUFFER_SIZE,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        .label = "caster manager buffer",
    };
    manager->buffer = wgpuDeviceCreateBuffer(resources->device, &buffer_desc);

    manager->dirty = false;
}

void caster_manager_free(CasterManager *manager)
{
    for (u32 i = 0; i < manager->entries.len; i++)
    {
        CasterEntry *entry = vec_get(&manager->entries, i);
        free(entry->cells);
        free((void *)entry->path);
    }
    vec_free(&manager->casters);
    vec_free(&manager->entries);
    wgpuBufferRelease(manager->buffer);
}

CasterEntry *caster_manager_load(CasterManager *manager, const char *path)
{
    for (u32 i = 0; i < manager->entries.len; i++)
    {
        CasterEntry *entry = vec_get(&manager->entries, i);
        if (strcmp(entry->path, path) == 0)
        {
            return entry;
        }
    }

    // looks like the caster hasn't been loaded yet
    manager->dirty = true;

    char out_err_msg[256];
    SHDWFile *shdw = shdw_parse(path, out_err_msg);
    if (!shdw)
    {
        FATAL("Failed to load SHDW file: %s", out_err_msg);
    }

    CasterEntry *entry = malloc(sizeof(CasterEntry));
    entry->path = strdup(path);
    entry->cell_count = shdw->cell_count;
    entry->cells = malloc(sizeof(CasterCell) * shdw->cell_count);

    u32 start = manager->casters.len;
    for (u32 i = 0; i < shdw->cell_count; i++)
    {
        Cell *cell = &shdw->cells[i];
        vec_resize(&manager->casters, manager->casters.len + cell->point_count);
        memcpy(manager->casters.data + start * sizeof(vec2s), cell->points,
               sizeof(vec2s) * cell->point_count);
        entry->cells[i].start = start;
        entry->cells[i].end = start + cell->point_count;
        start += cell->point_count;
    }
    vec_push(&manager->entries, &entry);

    shdw_free(shdw);
    return entry;
}

void caster_manager_clear(CasterManager *manager)
{
    for (u32 i = 0; i < manager->entries.len; i++)
    {
        CasterEntry *entry = vec_get(&manager->entries, i);
        free(entry->cells);
        free((void *)entry->path);
    }
    vec_clear(&manager->entries);
    vec_clear(&manager->casters);
    // don't bother writing any updates, all the data is
    // gone and should be reloaded
    manager->dirty = false;
}

void caster_manager_write_dirty(CasterManager *manager,
                                WGPUResources *resources)
{
    if (!manager->dirty)
        return;

    u32 buffer_size = wgpuBufferGetSize(manager->buffer);
    bool needs_regen = buffer_size < sizeof(vec2s) * manager->casters.cap;
    if (needs_regen)
    {
        wgpuBufferRelease(manager->buffer);
        WGPUBufferDescriptor buffer_desc = {
            .size = sizeof(vec2s) * manager->casters.cap,
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .label = "caster manager buffer",
        };
        manager->buffer =
            wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    }
    wgpuQueueWriteBuffer(resources->queue, manager->buffer, 0,
                         manager->casters.data,
                         sizeof(vec2s) * manager->casters.cap);
    manager->dirty = false;
}
