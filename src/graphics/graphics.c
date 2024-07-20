#include <stdio.h>
#include <wgpu.h>

#include "graphics.h"
#include "cglm/struct/clipspace/persp_rh_no.h"
#include "core_types.h"
#include "binding_helper.h"
#include "graphics/quad_manager.h"
#include "graphics/tex_manager.h"
#include "graphics/transform_manager.h"
#include "imgui-wgpu.h"
#include "input/input.h"
#include "utility/macros.h"
#include "webgpu.h"

void graphics_init(Graphics *graphics, SDL_Window *window)
{
    wgpu_resources_init(&graphics->wgpu, window);
    bing_group_layouts_init(&graphics->bind_group_layouts, &graphics->wgpu);
    shaders_init(&graphics->shaders, &graphics->bind_group_layouts,
                 &graphics->wgpu);

    quad_manager_init(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_init(&graphics->transform_manager, &graphics->wgpu);
    // texture manager has no gpu side resources allocated initially so no need
    // to pass wgpu
    texture_manager_init(&graphics->texture_manager);

    WGPUExtent3D extents = {
        .width = 640,
        .height = 480,
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

    texture_manager_load(&graphics->texture_manager,
                         "assets/textures/molly.png", &graphics->wgpu);

    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
    Rect rect = rect_from_min_size((vec2s){.x = 0., .y = 0.},
                                   (vec2s){.x = 200 * 1.33, .y = 200});
    Quad quad = {
        .rect = rect,
        .tex_coords = tex_coords,
    };
    quad_manager_add(&graphics->quad_manager, quad);

    Transform transform = transform_from_xyz(0.0, 0.0, 0.0);
    transform_manager_add(&graphics->transform_manager, transform);
    transform_manager_add(&graphics->transform_manager, transform);
}

int camera_x = 0;
int camera_y = 0;
int camera_z = 0;

void graphics_render(Graphics *graphics, Input *input)
{
    if (input_is_down(input, Button_Down))
        camera_z--;
    if (input_is_down(input, Button_Up))
        camera_z++;
    if (input_is_down(input, Button_Left))
        camera_x++;
    if (input_is_down(input, Button_Right))
        camera_x--;
    if (input_is_down(input, Button_Jump))
        camera_y++;
    if (input_is_down(input, Button_Crouch))
        camera_y--;

    Transform transform = transform_from_xyz(
        fmod(SDL_GetTicks() / 10.0, 640.0) - 320.0, 0.0, 0.0);
    transform_manager_update(&graphics->transform_manager, 1, transform);

    quad_manager_upload_dirty(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_upload_dirty(&graphics->transform_manager,
                                   &graphics->wgpu);

    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);
    bind_group_builder_append_texture_view_array(
        &builder,
        (WGPUTextureView *)graphics->texture_manager.texture_views.data,
        graphics->texture_manager.texture_views.len);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    WGPUBindGroup transform_bind_group = bind_group_build(
        &builder, graphics->wgpu.device, graphics->bind_group_layouts.object,
        "Transform Bind Group");

    bind_group_builder_free(&builder);

    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);
    bind_group_builder_append_texture_view(&builder, graphics->color_view);
    bind_group_builder_append_texture_view(&builder, graphics->normal_view);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    WGPUBindGroup light_bind_group = bind_group_build(
        &builder, graphics->wgpu.device, graphics->bind_group_layouts.lighting,
        "Light Bind Group");

    bind_group_builder_free(&builder);

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

    typedef struct
    {
        mat4s camera;
        u32 transform_index;
        u32 texture_index;
    } ObjectPushConstants;

    // bind pipeline and buffers
    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.object);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, transform_bind_group, 0,
                                      0);
    u32 buffer_size = wgpuBufferGetSize(graphics->quad_manager.buffer);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, buffer_size);

    mat4s camera_projection = glms_ortho(0.0, 640.0, 480.0, 0.0, -1.0f, 1.0f);
    mat4s camera_transform =
        glms_look((vec3s){.x = camera_x, .y = camera_y, .z = camera_z},
                  (vec3s){.x = 0.0, .y = 0.0, .z = -1.0},
                  (vec3s){.x = 0.0, .y = 1.0, .z = 0.0});
    mat4s camera = glms_mat4_mul(camera_projection, camera_transform);
    ObjectPushConstants push_constants = {
        .camera = camera,
        .transform_index = 0,
        .texture_index = 0,
    };
    wgpuRenderPassEncoderSetPushConstants(
        render_pass, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
        sizeof(ObjectPushConstants), &push_constants);
    wgpuRenderPassEncoderDraw(render_pass, VERTICES_PER_QUAD, 1,
                              QUAD_ENTRY_TO_VERTEX_INDEX(0), 0);

    wgpuRenderPassEncoderEnd(render_pass);
    wgpuRenderPassEncoderRelease(render_pass);

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
    render_pass = wgpuCommandEncoderBeginRenderPass(command_encoder,
                                                    &screen_render_pass_desc);

    typedef struct
    {
        mat4s camera;
        u32 transform_index;
        vec4s color;
    } LightPushCosntants;

    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.lighting);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, light_bind_group, 0, 0);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, buffer_size);

    LightPushCosntants light_push_constants = {
        .camera = camera,
        .transform_index = 1,
        .color = {.x = 1.0, .y = 1.0, .z = 1.0, .w = 1.0},
    };
    wgpuRenderPassEncoderSetPushConstants(
        render_pass, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
        sizeof(LightPushCosntants), &light_push_constants);

    wgpuRenderPassEncoderDraw(render_pass, VERTICES_PER_QUAD, 1,
                              QUAD_ENTRY_TO_VERTEX_INDEX(0), 0);

    ImGui_ImplWGPU_RenderDrawData(igGetDrawData(), render_pass);

    wgpuRenderPassEncoderEnd(render_pass);

    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, NULL);
    wgpuQueueSubmit(graphics->wgpu.queue, 1, &command_buffer);
    wgpuSurfacePresent(graphics->wgpu.surface);

    wgpuBindGroupRelease(transform_bind_group);

    wgpuCommandBufferRelease(command_buffer);
    wgpuRenderPassEncoderRelease(render_pass);
    wgpuCommandEncoderRelease(command_encoder);
    wgpuTextureViewRelease(frame);
    wgpuTextureRelease(surface_texture.texture);
}

void graphics_free(Graphics *graphics)
{
    quad_manager_free(&graphics->quad_manager);
    transform_manager_free(&graphics->transform_manager);
    texture_manager_free(&graphics->texture_manager);

    wgpuRenderPipelineRelease(graphics->shaders.object);
    wgpuBindGroupLayoutRelease(graphics->bind_group_layouts.object);

    wgpu_resources_free(&graphics->wgpu);
}
