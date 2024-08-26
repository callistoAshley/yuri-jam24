#include <stdio.h>
#include <wgpu.h>

#include "graphics.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#include "core_types.h"
#include "binding_helper.h"
#include "graphics/layer.h"
#include "graphics/quad_manager.h"
#include "graphics/sprite.h"
#include "graphics/tex_manager.h"
#include "graphics/tilemap.h"
#include "imgui-wgpu.h"
#include "input/input.h"
#include "physics/debug_draw.h"
#include "utility/log.h"
#include "utility/macros.h"
#include "utility/common_defines.h"
#include "webgpu.h"
#include "fonts/font.h"

QuadEntry screen_quad_index;
QuadEntry graphics_screen_quad_entry(void) { return screen_quad_index; }

void tilemap_layer_draw(void *layer, Graphics *graphics, mat4s camera,
                        WGPURenderPassEncoder pass)
{
    (void)graphics;
    TilemapLayer *tilemap_layer = layer;
    tilemap_render(tilemap_layer->tilemap, camera, tilemap_layer->layer, pass);
}

void sprite_draw(void *thing, Graphics *graphics, mat4s camera,
                 WGPURenderPassEncoder pass)
{
    (void)graphics;
    Sprite *sprite = thing;
    sprite_render(sprite, camera, pass);
}

void graphics_init(Graphics *graphics, SDL_Window *window)
{
    wgpu_resources_init(&graphics->wgpu, window);
    bind_group_layouts_init(&graphics->bind_group_layouts, &graphics->wgpu);
    shaders_init(&graphics->shaders, &graphics->bind_group_layouts,
                 &graphics->wgpu);

    quad_manager_init(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_init(&graphics->transform_manager, &graphics->wgpu);
    // texture manager has no gpu side resources allocated initially so no need
    // to pass wgpu
    texture_manager_init(&graphics->texture_manager);

    layer_init(&graphics->tilemap_layers.background, tilemap_layer_draw, free);
    layer_init(&graphics->tilemap_layers.middle, tilemap_layer_draw, free);
    layer_init(&graphics->tilemap_layers.foreground, tilemap_layer_draw, free);

    layer_init(&graphics->sprite_layers.background, sprite_draw, NULL);
    layer_init(&graphics->sprite_layers.middle, sprite_draw, NULL);
    layer_init(&graphics->sprite_layers.foreground, sprite_draw, NULL);

    layer_init(&graphics->ui_layers.background, sprite_draw, NULL);
    layer_init(&graphics->ui_layers.middle, sprite_draw, NULL);
    layer_init(&graphics->ui_layers.foreground, sprite_draw, NULL);

    // load bearing molly
    // why do we need this? primarily to make sure that at least one texture is
    // loaded, because you can't bind an empty texture array
    texture_manager_load(&graphics->texture_manager,
                         "assets/textures/load_bearing_molly.png",
                         &graphics->wgpu);

    WGPUExtent3D extents = {
        .width = INTERNAL_SCREEN_WIDTH,
        .height = INTERNAL_SCREEN_HEIGHT,
        .depthOrArrayLayers = 1,
    };
    WGPUTextureDescriptor desc = {
        .label = "color texture",
        .size = extents,
        .dimension = WGPUTextureDimension_2D,
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .usage =
            WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
    };
    graphics->color = wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
    graphics->color_view = wgpuTextureCreateView(graphics->color, NULL);

    desc.label = "normal texture";
    desc.format = WGPUTextureFormat_RGBA32Float;
    graphics->normal = wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
    graphics->normal_view = wgpuTextureCreateView(graphics->normal, NULL);

    graphics->sampler = wgpuDeviceCreateSampler(graphics->wgpu.device, NULL);

    // screen quad
    {
        Rect rect = {
            .min = {.x = -1.0, .y = 1.0},
            .max = {.x = 1.0, .y = -1.0},
        };
        Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
        Quad quad = {
            .rect = rect,
            .tex_coords = tex_coords,
        };
        screen_quad_index = quad_manager_add(&graphics->quad_manager, quad);
    }
}

void build_object_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);
    bind_group_builder_append_texture_view_array(
        &builder,
        (WGPUTextureView *)graphics->texture_manager.texture_views.data,
        graphics->texture_manager.texture_views.len);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.object,
                                   "Transform Bind Group");

    bind_group_builder_free(&builder);
}

void build_light_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_init(&builder);

    bind_group_builder_append_texture_view(&builder, graphics->color_view);
    bind_group_builder_append_texture_view(&builder, graphics->normal_view);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.lighting,
                                   "Light Bind Group");

    bind_group_builder_free(&builder);
}

void build_tilemap_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
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

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.tilemap,
                                   "Tilemap Bind Group");

    bind_group_builder_free(&builder);
}

void graphics_render(Graphics *graphics, Physics *physics, Camera raw_camera)
{
    quad_manager_upload_dirty(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_upload_dirty(&graphics->transform_manager,
                                   &graphics->wgpu);

    WGPUBindGroup object_bind_group;
    build_object_bind_group(graphics, &object_bind_group);

    WGPUBindGroup light_bind_group;
    build_light_bind_group(graphics, &light_bind_group);

    WGPUBindGroup tilemap_bind_group;
    build_tilemap_bind_group(graphics, &tilemap_bind_group);

    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(graphics->wgpu.surface, &surface_texture);

    switch (surface_texture.status)
    {
    case WGPUSurfaceGetCurrentTextureStatus_Success:
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
    {
        // Skip this frame, and re-configure surface.
        if (surface_texture.texture != NULL)
        {
            wgpuTextureRelease(surface_texture.texture);
        }
        wgpuSurfaceConfigure(graphics->wgpu.surface,
                             &graphics->wgpu.surface_config);

        return;
    }
    case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
    case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
    case WGPUSurfaceGetCurrentTextureStatus_Force32:
        FATAL("WGPU surface texture status: %#.8x\n", surface_texture.status);
    }

    WGPUTextureView frame =
        wgpuTextureCreateView(surface_texture.texture, NULL);

    WGPUCommandEncoder command_encoder =
        wgpuDeviceCreateCommandEncoder(graphics->wgpu.device, NULL);

    u64 quad_buffer_size = wgpuBufferGetSize(graphics->quad_manager.buffer);
    {
        WGPURenderPassColorAttachment object_attachments[] = {
            {
                .view = graphics->color_view,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f},
            },
            {
                .view = graphics->normal_view,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f},
            }};
        WGPURenderPassDescriptor object_render_pass_desc = {
            .label = "object render pass encoder",
            .colorAttachmentCount = 2,
            .colorAttachments = object_attachments,
        };
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
            command_encoder, &object_render_pass_desc);

        mat4s camera_projection =
            glms_ortho(0.0, INTERNAL_SCREEN_WIDTH, INTERNAL_SCREEN_HEIGHT, 0.0,
                       -1.0f, 1.0f);
        mat4s camera_transform = glms_look(
            (vec3s){.x = raw_camera.x, .y = raw_camera.y, .z = raw_camera.z},
            (vec3s){.x = 0.0, .y = 0.0, .z = -1.0},
            (vec3s){.x = 0.0, .y = 1.0, .z = 0.0});
        mat4s camera = glms_mat4_mul(camera_projection, camera_transform);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.background, graphics, camera,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.object);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, object_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        layer_draw(&graphics->sprite_layers.background, graphics, camera,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.middle, graphics, camera,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.object);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, object_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        layer_draw(&graphics->sprite_layers.middle, graphics, camera,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.foreground, graphics, camera,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.object);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, object_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        layer_draw(&graphics->sprite_layers.foreground, graphics, camera,
                   render_pass);

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
    }

    Box2DDebugCtx debug_ctx;
    if (physics->debug_draw)
        physics_debug_draw_init(&debug_ctx, graphics, raw_camera);

    {
        WGPURenderPassColorAttachment screen_attachments[] = {{
            .view = frame,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f},
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        }};
        WGPURenderPassDescriptor screen_render_pass_desc = {
            .label = "screen render pass encoder",
            .colorAttachmentCount = 1,
            .colorAttachments = screen_attachments,
        };
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
            command_encoder, &screen_render_pass_desc);
        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.lighting);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, light_bind_group, 0,
                                          0);

        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderDraw(render_pass, VERTICES_PER_QUAD, 1,
                                  QUAD_ENTRY_TO_VERTEX_INDEX(screen_quad_index),
                                  0);

        mat4s camera_projection =
            glms_ortho(0.0, graphics->wgpu.surface_config.width,
                       graphics->wgpu.surface_config.height, 0.0, -1.0f, 1.0f);
        mat4s camera_transform =
            glms_look(GLMS_VEC3_ZERO, (vec3s){.x = 0.0, .y = 0.0, .z = -1.0},
                      (vec3s){.x = 0.0, .y = 1.0, .z = 0.0});
        mat4s camera = glms_mat4_mul(camera_projection, camera_transform);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.ui_object);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, object_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);

        layer_draw(&graphics->ui_layers.background, graphics, camera,
                   render_pass);
        layer_draw(&graphics->ui_layers.middle, graphics, camera, render_pass);
        layer_draw(&graphics->ui_layers.foreground, graphics, camera,
                   render_pass);

        if (physics->debug_draw)
            physics_debug_draw(&debug_ctx, physics, render_pass);

        ImGui_ImplWGPU_RenderDrawData(igGetDrawData(), render_pass);

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
    }

    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, NULL);
    wgpuQueueSubmit(graphics->wgpu.queue, 1, &command_buffer);
    wgpuSurfacePresent(graphics->wgpu.surface);

    if (physics->debug_draw)
        physics_debug_draw_free(&debug_ctx);

    wgpuBindGroupRelease(object_bind_group);
    wgpuBindGroupRelease(light_bind_group);
    wgpuBindGroupRelease(tilemap_bind_group);

    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);
    wgpuTextureViewRelease(frame);
    wgpuTextureRelease(surface_texture.texture);
}

void graphics_free(Graphics *graphics)
{
    quad_manager_free(&graphics->quad_manager);
    transform_manager_free(&graphics->transform_manager);
    texture_manager_free(&graphics->texture_manager);

    layer_free(&graphics->tilemap_layers.background);
    layer_free(&graphics->tilemap_layers.middle);
    layer_free(&graphics->tilemap_layers.foreground);

    layer_free(&graphics->sprite_layers.background);
    layer_free(&graphics->sprite_layers.middle);
    layer_free(&graphics->sprite_layers.foreground);

    wgpuRenderPipelineRelease(graphics->shaders.object);
    wgpuBindGroupLayoutRelease(graphics->bind_group_layouts.object);

    wgpu_resources_free(&graphics->wgpu);
}

void graphics_resize(Graphics *graphics, int width, int height)
{
    graphics->wgpu.surface_config.width = width;
    graphics->wgpu.surface_config.height = height;
    wgpuSurfaceConfigure(graphics->wgpu.surface,
                         &graphics->wgpu.surface_config);
}
