#include <stdio.h>
#include <wgpu.h>

#include "graphics.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#include "core_types.h"
#include "binding_helper.h"
#include "graphics/caster_manager.h"
#include "graphics/layer.h"
#include "graphics/light.h"
#include "graphics/quad_manager.h"
#include "graphics/shaders.h"
#include "graphics/sprite.h"
#include "graphics/tex_manager.h"
#include "graphics/tilemap.h"
#include "imgui_wgpu.h"
#include "input/input.h"
#include "physics/debug_draw.h"
#include "utility/log.h"
#include "utility/macros.h"
#include "utility/common_defines.h"
#include "webgpu.h"
#include "fonts/font.h"

QuadEntry screen_quad_index;
QuadEntry graphics_screen_quad_entry(void) { return screen_quad_index; }

DirectionalLight directional_light;

CasterEntry *test;
CasterEntry *test2;

void tilemap_layer_draw(void *layer, void *context, Graphics *graphics,
                        WGPURenderPassEncoder pass)
{
    (void)graphics;
    TilemapLayer *tilemap_layer = layer;
    tilemap_render(tilemap_layer->tilemap, *(mat4s *)context,
                   tilemap_layer->layer, pass);
}

void sprite_draw(void *thing, void *context, Graphics *graphics,
                 WGPURenderPassEncoder pass)
{
    (void)graphics;
    Sprite *sprite = thing;
    sprite_render(sprite, *(mat4s *)context, pass);
}

void point_light_draw(void *thing, void *context, Graphics *graphics,
                      WGPURenderPassEncoder pass)
{
    (void)graphics;
    PointLight *light = thing;
    point_light_render(light, pass, *(Camera *)context);
}

void graphics_init(Graphics *graphics, SDL_Window *window)
{
    wgpu_resources_init(&graphics->wgpu, window);
    bind_group_layouts_init(&graphics->bind_group_layouts, &graphics->wgpu);
    shaders_init(&graphics->shaders, &graphics->bind_group_layouts,
                 &graphics->wgpu);

    quad_manager_init(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_init(&graphics->transform_manager, &graphics->wgpu);
    caster_manager_init(&graphics->caster_manager, &graphics->wgpu);
    // texture manager has no gpu side resources allocated initially so no need
    // to pass wgpu
    texture_manager_init(&graphics->texture_manager);

    test = caster_manager_load(&graphics->caster_manager,
                               "assets/shadowcasters/player.shdw");
    test2 = caster_manager_load(&graphics->caster_manager,
                                "assets/shadowcasters/red_start.shdw");

    layer_init(&graphics->tilemap_layers.background, tilemap_layer_draw, free);
    layer_init(&graphics->tilemap_layers.middle, tilemap_layer_draw, free);
    layer_init(&graphics->tilemap_layers.foreground, tilemap_layer_draw, free);

    layer_init(&graphics->sprite_layers.background, sprite_draw, NULL);
    layer_init(&graphics->sprite_layers.middle, sprite_draw, NULL);
    layer_init(&graphics->sprite_layers.foreground, sprite_draw, NULL);

    layer_init(&graphics->ui_layers.background, sprite_draw, NULL);
    layer_init(&graphics->ui_layers.middle, sprite_draw, NULL);
    layer_init(&graphics->ui_layers.foreground, sprite_draw, NULL);

    layer_init(&graphics->lights, point_light_draw, NULL);

    // load bearing molly
    // why do we need this? primarily to make sure that at least one texture is
    // loaded, because you can't bind an empty texture array
    texture_manager_load(&graphics->texture_manager,
                         "assets/textures/load_bearing_molly.png",
                         &graphics->wgpu);

    {
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
            .usage = WGPUTextureUsage_RenderAttachment |
                     WGPUTextureUsage_TextureBinding,
        };
        graphics->color = wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
        graphics->color_view = wgpuTextureCreateView(graphics->color, NULL);

        desc.label = "lit texture";
        desc.format = graphics->wgpu.surface_config.format;
        graphics->lit = wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
        graphics->lit_view = wgpuTextureCreateView(graphics->lit, NULL);
    }

    {
        WGPUExtent3D extents = {
            .width = 1024,
            .height = 64,
            .depthOrArrayLayers = 1,
        };
        WGPUTextureDescriptor desc = {
            .label = "shadowmap texture",
            .size = extents,
            .dimension = WGPUTextureDimension_2D,
            .format = WGPUTextureFormat_Depth24Plus,
            .mipLevelCount = 1,
            .sampleCount = 1,
            .usage = WGPUTextureUsage_RenderAttachment |
                     WGPUTextureUsage_TextureBinding,
        };
        graphics->shadows =
            wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
        graphics->shadows_view = wgpuTextureCreateView(graphics->shadows, NULL);
    }

    graphics->sampler = wgpuDeviceCreateSampler(graphics->wgpu.device, NULL);

    for (int i = 0; i < 10; i++)
    {
        PointLight *light = malloc(sizeof(PointLight));
        point_light_init(light, (vec3s){.x = i * 25.0 + 50.0, .y = 75.0},
                         (vec3s){.x = i / 10.0, .y = 0.1, .z = 1.0}, 50.0f);

        layer_add(&graphics->lights, light);
    }

    directional_light_init(&directional_light,
                           (vec3s){.x = 0.2, .y = 0.2, .z = 0.2}, 0.0);

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

void build_sprite_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
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
                                   graphics->bind_group_layouts.sprite,
                                   "Sprite Bind Group");

    bind_group_builder_free(&builder);
}

void build_light_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_init(&builder);

    bind_group_builder_append_texture_view(&builder, graphics->color_view);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.lighting,
                                   "Light Bind Group");

    bind_group_builder_free(&builder);
}

void build_tilemap_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    // FIXME it's a waste to create a new bind group every frame + this has the
    // same layout as the regular sprite bind group
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

void build_screen_blit_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_texture_view(&builder, graphics->lit_view);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.screen_blit,
                                   "Screen Blit Bind Group");

    bind_group_builder_free(&builder);
}

void build_shadowmapping_bind_group(Graphics *graphics,
                                    WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.shadowmapping,
                                   "Shadowmapping Bind Group");

    bind_group_builder_free(&builder);
}

void graphics_render(Graphics *graphics, Physics *physics, Camera raw_camera)
{
    quad_manager_upload_dirty(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_upload_dirty(&graphics->transform_manager,
                                   &graphics->wgpu);
    caster_manager_write_dirty(&graphics->caster_manager, &graphics->wgpu);

    // FIXME we really should not be creating a new bind group every frame
    WGPUBindGroup sprite_bind_group;
    build_sprite_bind_group(graphics, &sprite_bind_group);

    WGPUBindGroup light_bind_group;
    build_light_bind_group(graphics, &light_bind_group);

    WGPUBindGroup tilemap_bind_group;
    build_tilemap_bind_group(graphics, &tilemap_bind_group);

    WGPUBindGroup screen_blit_bind_group;
    build_screen_blit_bind_group(graphics, &screen_blit_bind_group);

    WGPUBindGroup shadowmapping_bind_group;
    build_shadowmapping_bind_group(graphics, &shadowmapping_bind_group);

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
    u64 caster_buffer_size = wgpuBufferGetSize(graphics->caster_manager.buffer);
    {
        WGPURenderPassColorAttachment defferred_attachments[] = {{
            .view = graphics->color_view,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f},
        }};
        WGPURenderPassDescriptor deferred_render_pass_desc = {
            .label = "defferred render pass encoder",
            .colorAttachmentCount = 1,
            .colorAttachments = defferred_attachments,
        };
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
            command_encoder, &deferred_render_pass_desc);

        mat4s camera_projection =
            glms_ortho(0.0, INTERNAL_SCREEN_WIDTH, INTERNAL_SCREEN_HEIGHT, 0.0,
                       -1.0f, 1.0f);
        mat4s camera_transform = glms_look(
            (vec3s){.x = raw_camera.x, .y = raw_camera.y, .z = raw_camera.z},
            (vec3s){.x = 0.0, .y = 0.0, .z = -1.0},
            (vec3s){.x = 0.0, .y = 1.0, .z = 0.0});
        mat4s camera = glms_mat4_mul(camera_projection, camera_transform);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.background, &camera, graphics,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);
        layer_draw(&graphics->sprite_layers.background, &camera, graphics,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.middle, &camera, graphics,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);
        layer_draw(&graphics->sprite_layers.middle, &camera, graphics,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.foreground, &camera, graphics,
                   render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);
        layer_draw(&graphics->sprite_layers.foreground, &camera, graphics,
                   render_pass);

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
    }

    // perform shadowmapping
    {
        WGPURenderPassDepthStencilAttachment depth_stencil = {
            .view = graphics->shadows_view,
            .depthClearValue = 1.0f,
            .depthLoadOp = WGPULoadOp_Clear,
            .depthStoreOp = WGPUStoreOp_Store,
        };
        WGPURenderPassDescriptor shadowmap_pass_desc = {
            .label = "shadowmap render pass encoder",
            .depthStencilAttachment = &depth_stencil,
        };
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
            command_encoder, &shadowmap_pass_desc);
        wgpuRenderPassEncoderSetViewport(render_pass, 0, 0, 160, 1, 0, 1);

        mat4s camera_projection = glms_ortho(0.0, 160, 1, 0.0, -1.0f, 200.0f);
        mat4s camera_transform =
            glms_look((vec3s){.x = 200.0, .y = -100.0, .z = 0.5},
                      (vec3s){.x = 0.0, .y = 1.0, .z = 0.0},
                      (vec3s){.x = 0.0, .y = 0.0, .z = -1.0});
        mat4s camera = glms_mat4_mul(camera_projection, camera_transform);

        wgpuRenderPassEncoderSetPipeline(
            render_pass, graphics->shaders.shadowmapping.sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0,
                                          shadowmapping_bind_group, 0, NULL);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0,
                                             graphics->caster_manager.buffer, 0,
                                             caster_buffer_size);

        SpriteShadowmapPushConstants push_constants = {
            .camera = camera,
            .transform_index = 3,
        };
        wgpuRenderPassEncoderSetPushConstants(
            render_pass, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
            sizeof(SpriteShadowmapPushConstants), &push_constants);

        CasterCell cell = test->cells[0];
        wgpuRenderPassEncoderDraw(render_pass, cell.end - cell.start, 1,
                                  cell.start, 0);

        wgpuRenderPassEncoderSetPipeline(
            render_pass, graphics->shaders.shadowmapping.tilemap);

        TilemapLayer *layer =
            *(TilemapLayer **)graphics->tilemap_layers.middle.entries.data;
        Tilemap *tilemap = layer->tilemap;

        for (i32 y = 0; y < tilemap->map_h; y++)
        {
            for (i32 x = 0; x < tilemap->map_w; x++)
            {
                i32 tile = tilemap->map_data[y * tilemap->map_w + x];
                if (tile < 0)
                    continue;

                CasterCell cell = test2->cells[tile];

                TilemapShadowmapPushConstants push_constants = {
                    .camera = camera,
                    .transform_index = tilemap->transform,
                    .tile_x = x,
                    .tile_y = y,
                };
                wgpuRenderPassEncoderSetPushConstants(
                    render_pass,
                    WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
                    sizeof(TilemapShadowmapPushConstants), &push_constants);

                wgpuRenderPassEncoderDraw(render_pass, cell.end - cell.start, 1,
                                          cell.start, 0);
            }
        }

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
    }

    // perform lighting
    {
        WGPURenderPassColorAttachment lit_attachments[] = {{
            .view = graphics->lit_view,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f},
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        }};
        WGPURenderPassDescriptor lit_render_pass_desc = {
            .label = "lit render pass encoder",
            .colorAttachmentCount = 1,
            .colorAttachments = lit_attachments,
        };
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
            command_encoder, &lit_render_pass_desc);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, light_bind_group, 0,
                                          0);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.lights.direct);
        directional_light_render(&directional_light, render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.lights.point);

        layer_draw(&graphics->lights, &raw_camera, graphics, render_pass);

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
    }

    Box2DDebugCtx debug_ctx;
    if (physics->debug_draw)
        physics_debug_draw_init(&debug_ctx, graphics, raw_camera);

    // actually draw to the screen
    {
        // we don't need to bother with clearing the screen because we're going
        // to be drawing over the entire thing anyway
        WGPURenderPassColorAttachment screen_attachments[] = {{
            .view = frame,
            .loadOp = WGPULoadOp_Load,
            .storeOp = WGPUStoreOp_Store,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        }};
        WGPURenderPassDescriptor screen_render_pass_desc = {
            .label = "screen render pass encoder",
            .colorAttachmentCount = 1,
            .colorAttachments = screen_attachments,
        };
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
            command_encoder, &screen_render_pass_desc);

        // copy the lit texture to the screen
        // we unfortunately can't use wgpuCommandEncoderCopyTextureToTexture
        // because the lit texture is not the same size as the screen texture
        // i wish there was some kind of blit function :( we could prooooobably
        // do this with a compute shader but i honestly could not be bothered
        // right now
        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.forward.screen_blit);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0,
                                          screen_blit_bind_group, 0, NULL);
        // no vertex buffer, just plain drawing
        wgpuRenderPassEncoderDraw(render_pass, 6, 1, 0, 0);

        if (physics->debug_draw)
            physics_debug_draw(&debug_ctx, physics, render_pass);

        mat4s camera_projection =
            glms_ortho(0.0, graphics->wgpu.surface_config.width,
                       graphics->wgpu.surface_config.height, 0.0, -1.0f, 1.0f);
        mat4s camera_transform =
            glms_look(GLMS_VEC3_ZERO, (vec3s){.x = 0.0, .y = 0.0, .z = -1.0},
                      (vec3s){.x = 0.0, .y = 1.0, .z = 0.0});
        mat4s camera = glms_mat4_mul(camera_projection, camera_transform);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.forward.ui_sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);

        layer_draw(&graphics->ui_layers.background, &camera, graphics,
                   render_pass);

        layer_draw(&graphics->ui_layers.middle, &camera, graphics, render_pass);

        // imgui is used for debug tools, so we want that to be on top of most
        // of the game's ui
        ImGui_ImplWGPU_RenderDrawData(igGetDrawData(), render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.forward.ui_sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);

        // imgui might set weird viewport and scissor rect values, so we need to
        // reset
        wgpuRenderPassEncoderSetViewport(
            render_pass, 0, 0, graphics->wgpu.surface_config.width,
            graphics->wgpu.surface_config.height, 0, 1);
        wgpuRenderPassEncoderSetScissorRect(
            render_pass, 0, 0, graphics->wgpu.surface_config.width,
            graphics->wgpu.surface_config.height);

        // however... we do want the foreground ui to be on top of imgui
        layer_draw(&graphics->ui_layers.foreground, &camera, graphics,
                   render_pass);

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
    }

    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, NULL);
    wgpuQueueSubmit(graphics->wgpu.queue, 1, &command_buffer);
    wgpuSurfacePresent(graphics->wgpu.surface);

    if (physics->debug_draw)
        physics_debug_draw_free(&debug_ctx);

    wgpuBindGroupRelease(sprite_bind_group);
    wgpuBindGroupRelease(light_bind_group);
    wgpuBindGroupRelease(tilemap_bind_group);
    wgpuBindGroupRelease(screen_blit_bind_group);
    wgpuBindGroupRelease(shadowmapping_bind_group);

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

    layer_free(&graphics->ui_layers.background);

    wgpuRenderPipelineRelease(graphics->shaders.defferred.sprite);
    wgpuBindGroupLayoutRelease(graphics->bind_group_layouts.sprite);

    wgpu_resources_free(&graphics->wgpu);
}

void graphics_resize(Graphics *graphics, int width, int height)
{
    graphics->wgpu.surface_config.width = width;
    graphics->wgpu.surface_config.height = height;
    wgpuSurfaceConfigure(graphics->wgpu.surface,
                         &graphics->wgpu.surface_config);
}
