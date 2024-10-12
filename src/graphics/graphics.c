#include <stdio.h>
#include <wgpu.h>

#include "graphics.h"
#include "cglm/types-struct.h"
#include "core_types.h"
#include "binding_helper.h"
#include "graphics/light.h"
#include "graphics/sprite.h"
#include "graphics/tilemap.h"
#include "graphics/ui_sprite.h"
#include "imgui_wgpu.h"
#include "input/input.h"
#include "physics/debug_draw.h"
#include "utility/log.h"
#include "utility/macros.h"
#include "webgpu.h"
#include "fonts/font.h"

// TODO remove all global variables
QuadEntry screen_quad_index;
QuadEntry graphics_screen_quad_entry(void) { return screen_quad_index; }

void tilemap_layer_draw(void *layer, void *context, WGPURenderPassEncoder pass)
{

    TilemapLayer *tilemap_layer = layer;
    tilemap_render(tilemap_layer->tilemap, *(mat4s *)context,
                   tilemap_layer->layer, pass);
}

void sprite_draw(void *thing, void *context, WGPURenderPassEncoder pass)
{
    Sprite *sprite = thing;
    sprite_render(sprite, *(mat4s *)context, pass);
}

void ui_sprite_draw(void *thing, void *context, WGPURenderPassEncoder pass)
{
    UiSprite *sprite = thing;
    ui_sprite_render(sprite, *(mat4s *)context, pass);
}

void point_light_draw(void *thing, void *context, WGPURenderPassEncoder pass)
{
    Light *light = thing;
    if (light->type != Light_Point)
        return;
    light_render(light, pass, *(Camera *)context);
}

void directional_light_draw(void *thing, void *context,
                            WGPURenderPassEncoder pass)
{
    Light *light = thing;
    if (light->type != Light_Direct)
        return;
    light_render(light, pass, *(Camera *)context);
}

struct ShadowCasterContext
{
    mat4s camera;
    vec2s light_position;
    vec2s camera_position;
    vec2s viewport_offset;
};
void shadowcaster_draw(void *thing, void *context, WGPURenderPassEncoder pass)
{
    ShadowCaster *caster = thing;
    struct ShadowCasterContext *caster_context = context;

    ShadowmapPushConstants constants = {
        .transform_index = caster->transform,
        .camera = caster_context->camera,
        .light_position = caster_context->light_position,

        .camera_position = caster_context->camera_position,
        .viewport_offset = caster_context->viewport_offset,
        .radius = caster->radius,
    };
    CasterCell cell = caster->caster->cells[caster->cell];

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
        sizeof(ShadowmapPushConstants), &constants);
    wgpuRenderPassEncoderDraw(pass, cell.end - cell.start, 1, cell.start, 0);
}

void graphics_init(Graphics *graphics, SDL_Window *window, Settings *settings)
{
    wgpu_resources_init(&graphics->wgpu, window, settings);
    bind_group_layouts_init(&graphics->bind_group_layouts, &graphics->wgpu);
    shaders_init(&graphics->shaders, &graphics->bind_group_layouts,
                 &graphics->wgpu);

    quad_manager_init(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_init(&graphics->transform_manager, &graphics->wgpu);
    caster_manager_init(&graphics->caster_manager, &graphics->wgpu);
    // texture manager has no gpu side resources allocated initially so no need
    // to pass wgpu
    texture_manager_init(&graphics->texture_manager);

    shadowmap_init(&graphics->shadowmap, &graphics->wgpu);

    layer_init(&graphics->tilemap_layers.background, tilemap_layer_draw);
    layer_init(&graphics->tilemap_layers.middle, tilemap_layer_draw);
    layer_init(&graphics->tilemap_layers.foreground, tilemap_layer_draw);

    layer_init(&graphics->sprite_layers.background, sprite_draw);
    layer_init(&graphics->sprite_layers.middle, sprite_draw);
    layer_init(&graphics->sprite_layers.foreground, sprite_draw);

    // TODO add free fns
    layer_init(&graphics->ui_layers.background, ui_sprite_draw);
    layer_init(&graphics->ui_layers.middle, ui_sprite_draw);
    layer_init(&graphics->ui_layers.foreground, ui_sprite_draw);

    layer_init(&graphics->lights, point_light_draw);
    layer_init(&graphics->shadowcasters, shadowcaster_draw);

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
    }

    {
        WGPUExtent3D extents = {
            .width = INTERNAL_SCREEN_WIDTH,
            .height = INTERNAL_SCREEN_HEIGHT,
            .depthOrArrayLayers = 1,
        };
        WGPUTextureDescriptor desc = {
            .label = "lit texture",
            .size = extents,
            .dimension = WGPUTextureDimension_2D,
            .format = WGPUTextureFormat_RGBA16Float,
            .mipLevelCount = 1,
            .sampleCount = 1,
            .usage = WGPUTextureUsage_RenderAttachment |
                     WGPUTextureUsage_TextureBinding,
        };
        graphics->lit = wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
        graphics->lit_view = wgpuTextureCreateView(graphics->lit, NULL);
    }

    WGPUSamplerDescriptor sampler_desc = {
        .addressModeU = WGPUAddressMode_Repeat,
        .addressModeV = WGPUAddressMode_Repeat,
        .addressModeW = WGPUAddressMode_Repeat,
        .label = "texture sampler",
        .maxAnisotropy = 1,
    };
    graphics->sampler =
        wgpuDeviceCreateSampler(graphics->wgpu.device, &sampler_desc);

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

    graphics->was_resized = false;
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

    bind_group_builder_append_texture_view(&builder, graphics->color_view);
    bind_group_builder_append_texture_view(&builder,
                                           graphics->shadowmap.texture_view);
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

void build_hdr_tonemap_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_texture_view(&builder, graphics->lit_view);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.hdr_tonemap,
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
                                   graphics->bind_group_layouts.shadowmap,
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

    WGPUBindGroup hdr_tonemap_bind_group;
    build_hdr_tonemap_bind_group(graphics, &hdr_tonemap_bind_group);

    WGPUBindGroup shadowmap_bind_hroup;
    build_shadowmapping_bind_group(graphics, &shadowmap_bind_hroup);

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

    mat4s camera_projection = glms_ortho(
        0.0, INTERNAL_SCREEN_WIDTH, INTERNAL_SCREEN_HEIGHT, 0.0, -1.0f, 1.0f);
    mat4s camera_transform = glms_look(
        (vec3s){.x = raw_camera.x, .y = raw_camera.y, .z = raw_camera.z},
        (vec3s){.x = 0.0, .y = 0.0, .z = -1.0},
        (vec3s){.x = 0.0, .y = 1.0, .z = 0.0});
    mat4s camera = glms_mat4_mul(camera_projection, camera_transform);

    u64 quad_buffer_size = wgpuBufferGetSize(graphics->quad_manager.buffer);
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

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.background, &camera, render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);
        layer_draw(&graphics->sprite_layers.background, &camera, render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.middle, &camera, render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);
        layer_draw(&graphics->sprite_layers.middle, &camera, render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.tilemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                          NULL);
        layer_draw(&graphics->tilemap_layers.foreground, &camera, render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.defferred.sprite);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, sprite_bind_group, 0,
                                          NULL);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
        wgpuRenderPassEncoderSetIndexBuffer(render_pass,
                                            graphics->quad_manager.index_buffer,
                                            WGPUIndexFormat_Uint16, 0, 12);
        layer_draw(&graphics->sprite_layers.foreground, &camera, render_pass);

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
    }

    // perform shadowmapping
    u32 caster_buffer_size = wgpuBufferGetSize(graphics->caster_manager.buffer);
    {
        WGPURenderPassColorAttachment attachments[] = {{
            .view = graphics->shadowmap.texture_view,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f},
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        }};
        WGPURenderPassDescriptor render_pass_desc = {
            .label = "shadowmap render pass encoder",
            .colorAttachmentCount = 1,
            .colorAttachments = attachments,
        };
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
            command_encoder, &render_pass_desc);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, shadowmap_bind_hroup,
                                          0, 0);
        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.lights.shadowmap);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0,
                                             graphics->caster_manager.buffer, 0,
                                             caster_buffer_size);

        ShadowMapIter iter;
        shadowmap_iter_init(&graphics->shadowmap, &iter);

        vec2s position;
        while (shadowmap_iter_next(&iter, &position))
        {
            vec2s tex_position =
                SHADOWMAP_ENTRY_POS_OFFSET(iter.current_entry - 1);
            wgpuRenderPassEncoderSetViewport(
                render_pass, tex_position.x, tex_position.y,
                INTERNAL_SCREEN_WIDTH, INTERNAL_SCREEN_HEIGHT, 0, 1);

            struct ShadowCasterContext context = {
                .camera = camera,
                .light_position = position,
                .camera_position = {.x = raw_camera.x, .y = raw_camera.y},
                .viewport_offset = tex_position};

            layer_draw(&graphics->shadowcasters, &context, render_pass);
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

        graphics->lights.draw = directional_light_draw;
        layer_draw(&graphics->lights, &raw_camera, render_pass);

        wgpuRenderPassEncoderSetPipeline(render_pass,
                                         graphics->shaders.lights.point);

        graphics->lights.draw = point_light_draw;
        layer_draw(&graphics->lights, &raw_camera, render_pass);

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
                                         graphics->shaders.forward.hdr_tonemap);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0,
                                          hdr_tonemap_bind_group, 0, NULL);
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

        layer_draw(&graphics->ui_layers.background, &camera, render_pass);

        layer_draw(&graphics->ui_layers.middle, &camera, render_pass);

        layer_draw(&graphics->ui_layers.foreground, &camera, render_pass);

        // imgui is used for debug tools, so we want that to be on top of most
        // of the game's ui
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

    wgpuBindGroupRelease(sprite_bind_group);
    wgpuBindGroupRelease(light_bind_group);
    wgpuBindGroupRelease(tilemap_bind_group);
    wgpuBindGroupRelease(hdr_tonemap_bind_group);
    wgpuBindGroupRelease(shadowmap_bind_hroup);

    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);
    wgpuTextureViewRelease(frame);
    wgpuTextureRelease(surface_texture.texture);

    graphics->was_resized = false;
}

void graphics_free(Graphics *graphics)
{
    quad_manager_free(&graphics->quad_manager);
    transform_manager_free(&graphics->transform_manager);
    texture_manager_free(&graphics->texture_manager);
    caster_manager_free(&graphics->caster_manager);

    layer_free(&graphics->tilemap_layers.background);
    layer_free(&graphics->tilemap_layers.middle);
    layer_free(&graphics->tilemap_layers.foreground);

    layer_free(&graphics->sprite_layers.background);
    layer_free(&graphics->sprite_layers.middle);
    layer_free(&graphics->sprite_layers.foreground);

    layer_free(&graphics->ui_layers.background);
    layer_free(&graphics->ui_layers.middle);
    layer_free(&graphics->ui_layers.foreground);

    layer_free(&graphics->lights);
    layer_free(&graphics->shadowcasters);

    shaders_free(&graphics->shaders);
    bind_group_layouts_free(&graphics->bind_group_layouts);

    wgpuSamplerRelease(graphics->sampler);

    wgpuTextureViewRelease(graphics->color_view);
    wgpuTextureRelease(graphics->color);

    wgpuTextureViewRelease(graphics->lit_view);
    wgpuTextureRelease(graphics->lit);

    shadowmap_free(&graphics->shadowmap);

    wgpu_resources_free(&graphics->wgpu);
}

void graphics_resize(Graphics *graphics, int width, int height)
{
    graphics->wgpu.surface_config.width = width;
    graphics->wgpu.surface_config.height = height;
    wgpuSurfaceConfigure(graphics->wgpu.surface,
                         &graphics->wgpu.surface_config);

    graphics->was_resized = true;
}
