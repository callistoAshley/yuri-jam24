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
    Vertex vertex[VERTICES_PER_QUAD];
    struct
    {
        // will be ENTRY_FREE if the entry is free
        u32 is_free;
        QuadEntry next;
    } next;
} QuadEntryData;

#define INITIAL_BUFFER_CAP 32
#define INITIAL_BUFFER_SIZE sizeof(QuadEntryData) * INITIAL_BUFFER_CAP

void quad_manager_init(QuadManager *manager, WGPUResources *resources)
{
    WGPUBufferDescriptor buffer_desc = {
        .size = INITIAL_BUFFER_SIZE,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        .label = "quad manager buffer",
    };
    manager->buffer = wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    vec_init_with_capacity(&manager->entries, sizeof(QuadEntryData),
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
    Vertex vertices[VERTICES_PER_QUAD];
    quad_into_vertices(quad, vertices);

    QuadEntry key = manager->next;
    manager->dirty = true;

    if (manager->next == manager->entries.len)
    {
        QuadEntryData entry;
        memcpy(entry.vertex, vertices, sizeof(vertices));
        assert(entry.next.is_free != ENTRY_FREE);
        vec_push(&manager->entries, &entry);
        manager->next++;
    }
    else
    {
        QuadEntryData *entry = vec_get(&manager->entries, manager->next);
        assert(entry != NULL);
        assert(entry->next.is_free == ENTRY_FREE);
        manager->next = entry->next.next;
        memcpy(entry->vertex, vertices, sizeof(vertices));
    }

    return key;
}

void quad_manager_remove(QuadManager *manager, QuadEntry entry)
{
    QuadEntryData *data = vec_get(&manager->entries, entry);
    assert(data != NULL);
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
    assert(data != NULL);
    assert(data->next.is_free != ENTRY_FREE);

    Vertex vertices[VERTICES_PER_QUAD];
    quad_into_vertices(quad, vertices);
    memcpy(data->vertex, vertices, sizeof(vertices));
    manager->dirty = true;
}

// ---  ---

void quad_manager_upload_dirty(QuadManager *manager, WGPUResources *resources)
{
    if (!manager->dirty)
        return;
    manager->dirty = false;

    u32 buffer_size = wgpuBufferGetSize(manager->buffer);
    bool needs_regen =
        manager->entries.cap * sizeof(QuadEntryData) > buffer_size;
    if (needs_regen)
    {
        wgpuBufferRelease(manager->buffer);
        WGPUBufferDescriptor buffer_desc = {
            // make sure the buffer has the same size as the array capacity.
            // if the array capacity ever grows, it's always double the previous
            // size.
            // previously this code would do this based on the array
            // length, which would work
            // but would require more resizing operations.
            .size = manager->entries.cap,
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .label = "quad manager buffer",
        };
        manager->buffer =
            wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    }
    // While this means we're writing
    // uninitialized memory, that memory
    // shouldn't be touched without a use-before-init bug.
    wgpuQueueWriteBuffer(resources->queue, manager->buffer, 0,
                         manager->entries.data,
                         manager->entries.cap * sizeof(QuadEntryData));
}
