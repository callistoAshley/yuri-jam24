#include "caster_manager.h"
#include "parsers/shdw.h"
#include "utility/macros.h"

#define INITIAL_BUFFER_CAP 256
#define INITIAL_BUFFER_SIZE sizeof(vec3s) * INITIAL_BUFFER_CAP

void caster_manager_init(CasterManager *manager, WGPUResources *resources)
{
    vec_init(&manager->casters, sizeof(vec3s));
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
    caster_manager_clear(manager);
    vec_free(&manager->casters);
    vec_free(&manager->entries);
    wgpuBufferRelease(manager->buffer);
}

CasterEntry *caster_manager_load(CasterManager *manager, const char *path)
{

    // looks like the caster hasn't been loaded yet

    char out_err_msg[256];
    SHDWFile *shdw = shdw_parse(path, out_err_msg);
    if (!shdw)
    {
        FATAL("Failed to load SHDW file: %s", out_err_msg);
    }

    CasterEntry *entry =
        caster_manager_register(manager, shdw->cells, shdw->cell_count);

    shdw_free(shdw);
    return entry;
}

CasterEntry *caster_manager_register(CasterManager *manager, Cell *cells,
                                     u32 cell_count)
{
    manager->dirty = true;

    CasterEntry *entry = malloc(sizeof(CasterEntry));
    entry->cell_count = cell_count;
    entry->cells = calloc(cell_count, sizeof(CasterCell));

    // upload everything as vec3 triangle strips
    for (u32 i = 0; i < cell_count; i++)
    {
        Cell *cell = &cells[i];
        entry->cells[i].start = manager->casters.len;
        for (u32 j = 0; j < cell->line_count; j++)
        {
            // 0
            vec3s point;
            point.x = cell->lines[j].start.x;
            point.y = cell->lines[j].start.y;
            point.z = 0;
            vec_push(&manager->casters, &point);

            // 1
            point.x = cell->lines[j].end.x;
            point.y = cell->lines[j].end.y;
            point.z = 0;
            vec_push(&manager->casters, &point);

            // 2
            point.x = cell->lines[j].start.x;
            point.y = cell->lines[j].start.y;
            point.z = 1;
            vec_push(&manager->casters, &point);

            // 2
            vec_push(&manager->casters, &point);

            // 1
            point.x = cell->lines[j].end.x;
            point.y = cell->lines[j].end.y;
            point.z = 0;
            vec_push(&manager->casters, &point);

            // 3
            point.x = cell->lines[j].end.x;
            point.y = cell->lines[j].end.y;
            point.z = 1;
            vec_push(&manager->casters, &point);
        }
        entry->cells[i].end = manager->casters.len;
    }
    vec_push(&manager->entries, &entry);

    return entry;
}

void caster_manager_clear(CasterManager *manager)
{
    for (u32 i = 0; i < manager->entries.len; i++)
    {
        CasterEntry *entry = *(CasterEntry **)vec_get(&manager->entries, i);
        free(entry->cells);
        free(entry);
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
    bool needs_regen = buffer_size < sizeof(vec3s) * manager->casters.cap;
    if (needs_regen)
    {
        wgpuBufferRelease(manager->buffer);
        WGPUBufferDescriptor buffer_desc = {
            .size = sizeof(vec3s) * manager->casters.cap,
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .label = "caster manager buffer",
        };
        manager->buffer =
            wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    }
    wgpuQueueWriteBuffer(resources->queue, manager->buffer, 0,
                         manager->casters.data,
                         sizeof(vec3s) * manager->casters.cap);
    manager->dirty = false;
}
