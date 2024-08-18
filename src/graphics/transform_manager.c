#include "transform_manager.h"
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
    mat4s transform;
    struct
    {
        // will be TRANSFORM_ENTRY_FREE if the entry is free
        u32 is_free;
        TransformEntry next;
    } next;
} TransformEntryData;

#define INITIAL_BUFFER_CAP 32
#define INITIAL_BUFFER_SIZE sizeof(TransformEntryData) * INITIAL_BUFFER_CAP

void transform_manager_init(TransformManager *manager, WGPUResources *resources)
{
    WGPUBufferDescriptor buffer_desc = {
        .size = INITIAL_BUFFER_SIZE,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage,
        .label = "Transform manager buffer",
    };
    manager->buffer = wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    vec_init_with_capacity(&manager->entries, sizeof(TransformEntryData),
                           INITIAL_BUFFER_CAP);
    manager->next = 0;
    manager->dirty = false;
}

void transform_manager_free(TransformManager *manager)
{
    wgpuBufferRelease(manager->buffer);
    vec_free(&manager->entries);
}

// ---  ---

TransformEntry transform_manager_add(TransformManager *manager,
                                     Transform Transform)
{
    mat4s matrix = transform_into_matrix(Transform);

    TransformEntry key = manager->next;
    manager->dirty = true;

    if (manager->next == manager->entries.len)
    {
        TransformEntryData entry;
        entry.transform = matrix;
        assert(entry.next.is_free != TRANSFORM_ENTRY_FREE);
        vec_push(&manager->entries, &entry);
        manager->next++;
    }
    else
    {
        TransformEntryData *entry = vec_get(&manager->entries, manager->next);
        assert(entry != NULL);
        assert(entry->next.is_free == TRANSFORM_ENTRY_FREE);
        manager->next = entry->next.next;
        entry->transform = matrix;
    }

    return key;
}

void transform_manager_remove(TransformManager *manager, TransformEntry entry)
{
    TransformEntryData *data = vec_get(&manager->entries, entry);
    assert(data != NULL);
    assert(data->next.is_free != TRANSFORM_ENTRY_FREE);

    data->next.is_free = TRANSFORM_ENTRY_FREE;
    data->next.next = manager->next;
    manager->next = entry;

    // we don't need to mark dirty here, because the entry is just marked as
    // free
}

void transform_manager_update(TransformManager *manager, TransformEntry entry,
                              Transform Transform)
{
    TransformEntryData *data = vec_get(&manager->entries, entry);
    assert(data != NULL);
    assert(data->next.is_free != TRANSFORM_ENTRY_FREE);

    mat4s matrix = transform_into_matrix(Transform);
    data->transform = matrix;
    manager->dirty = true;
}

// ---  ---

bool transform_manager_upload_dirty(TransformManager *manager,
                                    WGPUResources *resources)
{
    if (!manager->dirty)
        return false;
    manager->dirty = false;

    u32 buffer_size = wgpuBufferGetSize(manager->buffer);
    bool needs_regen =
        manager->entries.cap * sizeof(TransformEntryData) > buffer_size;
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
            .size = manager->entries.cap * sizeof(TransformEntryData),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .label = "Transform manager buffer",
        };
        manager->buffer =
            wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    }
    // While this means we're writing
    // uninitialized memory, that memory
    // shouldn't be touched without a use-before-init bug.
    wgpuQueueWriteBuffer(resources->queue, manager->buffer, 0,
                         manager->entries.data,
                         manager->entries.cap * sizeof(TransformEntryData));

    return needs_regen;
}
