#include "quad_manager.h"
#include "core_types.h"
#include "utility/vec.h"
#include "webgpu.h"

// why use a union?
// well, we want to upload the entire entry list to the gpu whenever the vertex
// buffer is dirty, which means it must be layout-compatible with Vertex.
// but, we also want to be able to mark entries as free, so we need to store
// some extra data in the entry list.
// a union lets us do both.
typedef union
{
    Vertex vertex;
    struct
    {
        // will be ENTRY_FREE if the entry is free
        u32 is_free;
        QuadEntry next;
    } next;
} QuadEntryData;

#define INITIAL_BUFFER_CAP 6 * 32
#define INITIAL_BUFFER_SIZE sizeof(Vertex) * INITIAL_BUFFER_CAP

void quad_manager_init(QuadManager *manager, WGPUResources *resources)
{
    WGPUBufferDescriptor buffer_desc = {
        .size = INITIAL_BUFFER_SIZE,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        .label = "quad manager buffer",
    };
    manager->buffer = wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    vec_init_with_capacity(&manager->entries, sizeof(Vertex),
                           INITIAL_BUFFER_CAP);
    manager->next = 0;
    manager->dirty = false;
}

void quad_manager_free(QuadManager *manager)
{
    wgpuBufferRelease(manager->buffer);
    vec_free(&manager->entries);
}

// ---  ---

QuadEntry quad_manager_add(QuadManager *manager, Quad quad)
{
    Vertex vertices[6];
    quad_into_vertices(quad, vertices);

    QuadEntry key = manager->next;
    manager->dirty = true;

    if (manager->next == manager->entries.len)
    {
        for (int i = 0; i < 6; i++)
        {
            QuadEntryData entry = {.vertex = vertices[i]};
            assert(entry.next.is_free != ENTRY_FREE);
            vec_push(&manager->entries, &entry);
        }
        manager->next += 6;
    }
    else
    {
        QuadEntryData *entry = vec_get(&manager->entries, manager->next);
        assert(entry->next.is_free == ENTRY_FREE);
        for (int i = 0; i < 6; i++)
        {
            entry[i].vertex = vertices[i];
        }
        manager->next += 6;
    }

    return key;
}

void quad_manager_remove(QuadManager *manager, QuadEntry entry)
{
    QuadEntryData *data = vec_get(&manager->entries, entry);
    assert(data->next.is_free != ENTRY_FREE);

    data->next.is_free = ENTRY_FREE;
    data->next.next = manager->next;
    manager->next = entry;

    // we don't need to mark dirty here, because the entry is just marked as
    // free
}

void quad_manager_update(QuadManager *manager, QuadEntry entry, Quad quad)
{
    QuadEntryData *data = vec_get(&manager->entries, entry);
    assert(data->next.is_free != ENTRY_FREE);

    Vertex vertices[6];
    quad_into_vertices(quad, vertices);

    for (int i = 0; i < 6; i++)
    {
        data[i].vertex = vertices[i];
    }

    manager->dirty = true;
}

// ---  ---

void quad_manager_upload_dirty(QuadManager *manager, WGPUResources *resources)
{
    if (!manager->dirty)
        return;

    u32 buffer_size = wgpuBufferGetSize(manager->buffer);
    if (manager->entries.len > buffer_size)
    {
        wgpuBufferRelease(manager->buffer);
        WGPUBufferDescriptor buffer_desc = {
            // multiply by two to avoid resizing too often
            .size = manager->entries.len * sizeof(Vertex) * 2,
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .label = "quad manager buffer",
        };
        manager->buffer =
            wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    }
    wgpuQueueWriteBuffer(resources->queue, manager->buffer, 0,
                         manager->entries.data,
                         manager->entries.len * sizeof(Vertex));
}
