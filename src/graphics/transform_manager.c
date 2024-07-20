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
        // will be ENTRY_FREE if the entry is free
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
        assert(entry.next.is_free != ENTRY_FREE);
        vec_push(&manager->entries, &entry);
    }
    else
    {
        TransformEntryData *entry = vec_get(&manager->entries, manager->next);
        assert(entry->next.is_free == ENTRY_FREE);
        entry->transform = matrix;
    }
    manager->next++;

    return key;
}

void transform_manager_remove(TransformManager *manager, TransformEntry entry)
{
    TransformEntryData *data = vec_get(&manager->entries, entry);
    assert(data->next.is_free != ENTRY_FREE);

    data->next.is_free = ENTRY_FREE;
    data->next.next = manager->next;
    manager->next = entry;

    // we don't need to mark dirty here, because the entry is just marked as
    // free
}

void transform_manager_update(TransformManager *manager, TransformEntry entry,
                              Transform Transform)
{
    TransformEntryData *data = vec_get(&manager->entries, entry);
    assert(data->next.is_free != ENTRY_FREE);

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

    u32 buffer_size = wgpuBufferGetSize(manager->buffer);
    bool needs_regen =
        manager->entries.len * sizeof(TransformEntryData) > buffer_size;
    if (needs_regen)
    {
        wgpuBufferRelease(manager->buffer);
        WGPUBufferDescriptor buffer_desc = {
            // multiply by two to avoid resizing too often
            .size = manager->entries.len * sizeof(TransformEntryData) * 2,
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .label = "Transform manager buffer",
        };
        manager->buffer =
            wgpuDeviceCreateBuffer(resources->device, &buffer_desc);
    }
    wgpuQueueWriteBuffer(resources->queue, manager->buffer, 0,
                         manager->entries.data,
                         manager->entries.len * sizeof(TransformEntryData));

    return needs_regen;
}
