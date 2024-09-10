#include "tilemap.h"
#include "graphics/binding_helper.h"
#include "graphics/shaders.h"
#include "graphics/tex_manager.h"
#include "graphics/transform_manager.h"
#include "utility/macros.h"
#include "utility/log.h"
#include "webgpu.h"

void tilemap_init(Tilemap *tilemap, Graphics *graphics, TextureEntry *tileset,
                  TransformEntry transform, int map_w, int map_h, int layers,
                  i32 *map_data)
{
    tilemap->tileset = tileset;
    tilemap->transform = transform;
    tilemap->map_w = map_w;
    tilemap->map_h = map_h;
    tilemap->layers = layers;

    usize map_data_size = map_w * map_h * layers * sizeof(i32);
    log_info("map data size: %lu", map_data_size);

    tilemap->normal_tex = texture_manager_load(
        &graphics->texture_manager, "assets/textures/red_start_normal.png",
        &graphics->wgpu);

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

void tilemap_render(Tilemap *tilemap, mat4s camera, int layer,
                    WGPURenderPassEncoder pass)
{
    u64 layer_size = tilemap->map_w * tilemap->map_h * sizeof(u32);
    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, tilemap->instances,
                                         layer_size * layer, layer_size);

    i32 normal_index = -1;
    if (tilemap->normal_tex)
        normal_index = tilemap->normal_tex->index;

    TilemapPushConstants constants = {
        .camera = camera,
        .transform_index = tilemap->transform,
        .texture_index = tilemap->tileset->index,
        .normal_texture_index = normal_index,
        .map_width = tilemap->map_w,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
        sizeof(TilemapPushConstants), &constants);
    wgpuRenderPassEncoderDraw(pass, VERTICES_PER_QUAD,
                              tilemap->map_w * tilemap->map_h, 0, 0);
}

void tilemap_set_tile(Tilemap *tilemap, Graphics *graphics, int x, int y,
                      int layer, i32 tile)
{
    u64 byte_offset =
        (y * tilemap->map_w + x + (tilemap->map_w * tilemap->map_h) * layer) *
        sizeof(i32);
    wgpuQueueWriteBuffer(graphics->wgpu.queue, tilemap->instances, byte_offset,
                         &tile, sizeof(i32));
}
