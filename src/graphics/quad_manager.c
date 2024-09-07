#include "quad_manager.h"
#include "core_types.h"
#include "utility/hashset.h"
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
        // will be QUAD_ENTRY_FREE if the entry is free
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
    hashset_init(&manager->dirty_entries, fnv_hash_function, memcmp_eq_function,
                 sizeof(QuadEntry));
}

void quad_manager_free(QuadManager *manager)
{
    wgpuBufferRelease(manager->buffer);
    vec_free(&manager->entries);
    hashset_free(&manager->dirty_entries);
}

// ---  ---

QuadEntry quad_manager_add(QuadManager *manager, Quad quad)
{
    Vertex vertices[VERTICES_PER_QUAD];
    quad_into_vertices(quad, vertices);

    QuadEntry key = manager->next;
    hashset_insert(&manager->dirty_entries, &key);

    if (manager->next == manager->entries.len)
    {
        QuadEntryData entry;
        memcpy(entry.vertex, vertices, sizeof(vertices));
        assert(entry.next.is_free != QUAD_ENTRY_FREE);
        vec_push(&manager->entries, &entry);
        manager->next++;
    }
    else
    {
        QuadEntryData *entry = vec_get(&manager->entries, manager->next);
        assert(entry != NULL);
        assert(entry->next.is_free == QUAD_ENTRY_FREE);
        manager->next = entry->next.next;
        memcpy(entry->vertex, vertices, sizeof(vertices));
    }

    return key;
}

void quad_manager_remove(QuadManager *manager, QuadEntry entry)
{
    QuadEntryData *data = vec_get(&manager->entries, entry);
    assert(data != NULL);
    assert(data->next.is_free != QUAD_ENTRY_FREE);

    data->next.is_free = QUAD_ENTRY_FREE;
    data->next.next = manager->next;
    manager->next = entry;

    // remove the entry from the dirty set (this is really not a common case, we
    // could probably omit this)
    hashset_remove(&manager->dirty_entries, &entry);
}

void quad_manager_update(QuadManager *manager, QuadEntry entry, Quad quad)
{
    QuadEntryData *data = vec_get(&manager->entries, entry);
    assert(data != NULL);
    assert(data->next.is_free != QUAD_ENTRY_FREE);

    Vertex vertices[VERTICES_PER_QUAD];
    quad_into_vertices(quad, vertices);
    memcpy(data->vertex, vertices, sizeof(vertices));
    hashset_insert(&manager->dirty_entries, &entry);
}

Quad quad_manager_get(QuadManager *manager, QuadEntry entry)
{
    QuadEntryData *data = vec_get(&manager->entries, entry);
    assert(data != NULL);
    assert(data->next.is_free != QUAD_ENTRY_FREE);

    Vertex vertices[VERTICES_PER_QUAD];
    memcpy(vertices, data->vertex, sizeof(vertices));
    return quad_from_vertices(vertices);
}

// ---  ---

void quad_manager_upload_dirty(QuadManager *manager, WGPUResources *resources)
{
    if (manager->dirty_entries.len == 0)
        return;

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
            .size = manager->entries.cap * sizeof(QuadEntryData),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .label = "quad manager buffer",
        };
        manager->buffer =
            wgpuDeviceCreateBuffer(resources->device, &buffer_desc);

        // copy all the entries to the new buffer and reset the dirty set
        wgpuQueueWriteBuffer(resources->queue, manager->buffer, 0,
                             manager->entries.data,
                             manager->entries.len * sizeof(QuadEntryData));
        hashset_clear(&manager->dirty_entries);

        return;
    }

    // write all the dirty entries to the buffer
    HashSetIter iter;
    hashset_iter_init(&manager->dirty_entries, &iter);
    QuadEntry *entry = hashset_iter_next(&iter);

    while (entry != NULL)
    {
        QuadEntryData *data = vec_get(&manager->entries, *entry);
        assert(data != NULL);
        assert(data->next.is_free != QUAD_ENTRY_FREE);

        wgpuQueueWriteBuffer(resources->queue, manager->buffer,
                             *entry * sizeof(QuadEntryData), data->vertex,
                             sizeof(data->vertex));

        entry = hashset_iter_next(&iter);
    }

    // clear the dirty set
    hashset_clear(&manager->dirty_entries);
}
