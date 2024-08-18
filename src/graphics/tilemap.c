#include "tilemap.h"
#include "graphics/binding_helper.h"
#include "graphics/shaders.h"
#include "graphics/tex_manager.h"
#include "graphics/transform_manager.h"
#include "utility/macros.h"
#include "utility/log.h"
#include "webgpu.h"

void tilemap_new(Tilemap *tilemap, Graphics *graphics, TextureEntry *tileset,
                 TransformEntry transform, int map_w, int map_h, int layers,
                 u32 *map_data)
{
    tilemap->tileset = tileset;
    tilemap->transform = transform;
    tilemap->map_w = map_w;
    tilemap->map_h = map_h;
    tilemap->layers = layers;

    usize map_data_size = map_w * map_h * layers * sizeof(u32);
    log_info("map data size: %lu", map_data_size);

    WGPUBufferDescriptor buffer_desc = {
        .label = "tilemap instance buffer",
        .size = map_data_size,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
    };
    tilemap->instances =
        wgpuDeviceCreateBuffer(graphics->wgpu.device, &buffer_desc);
    wgpuQueueWriteBuffer(graphics->wgpu.queue, tilemap->instances, 0, map_data,
                         map_data_size);
}

void tilemap_free(Tilemap *tilemap, Graphics *graphics)
{
    wgpuBufferRelease(tilemap->instances);
    texture_manager_unload(&graphics->texture_manager, tilemap->tileset);
    transform_manager_remove(&graphics->transform_manager, tilemap->transform);
}

void tilemap_render(Tilemap *tilemap, Graphics *graphics, mat4s camera,
                    WGPURenderPassEncoder pass)
{
    // FIXME it's a waste to create a new bind group every frame + this has the
    // same layout as the regular object bind group
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);
    bind_group_builder_append_texture_view_array(
        &builder,
        (WGPUTextureView *)graphics->texture_manager.texture_views.data,
        graphics->texture_manager.texture_views.len);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    WGPUBindGroup bind_group = bind_group_build(
        &builder, graphics->wgpu.device, graphics->bind_group_layouts.tilemap,
        "Tilemap Bind Group");

    bind_group_builder_free(&builder);

    wgpuRenderPassEncoderSetPipeline(pass, graphics->shaders.tilemap);

    // bind pipeline and buffers
    wgpuRenderPassEncoderSetBindGroup(pass, 0, bind_group, 0, 0);
    u32 buffer_size = wgpuBufferGetSize(tilemap->instances);
    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, tilemap->instances, 0,
                                         buffer_size);

    TilemapPushConstants constants = {
        .camera = camera,
        .transform_index = tilemap->transform,
        .texture_index = tilemap->tileset->index,
        .map_width = tilemap->map_w,
        .map_height = tilemap->map_h,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
        sizeof(TilemapPushConstants), &constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD,
                              tilemap->map_w * tilemap->map_h * tilemap->layers,
                              0, 0);

    // wgpuBindGroupRelease(bind_group);
}

void tilemap_set_tile(Tilemap *tilemap, Graphics *graphics, int x, int y,
                      int layer, u16 tile)
{
    u64 byte_offset =
        (y * tilemap->map_w + x + (tilemap->map_w * tilemap->map_h) * layer) *
        sizeof(u16);
    wgpuQueueWriteBuffer(graphics->wgpu.queue, tilemap->instances, byte_offset,
                         &tile, sizeof(u16));
}
