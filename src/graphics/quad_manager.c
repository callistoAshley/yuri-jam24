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
        u32 next;
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
