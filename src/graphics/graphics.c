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

    texture_manager_load(&graphics->texture_manager,
                         "assets/textures/molly.png", &graphics->wgpu);

    graphics->sampler = wgpuDeviceCreateSampler(graphics->wgpu.device, NULL);

    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
    Rect rect = rect_from_min_size((vec2s){.x = 0., .y = 0.},
                                   (vec2s){.x = 64, .y = 64});
    Quad quad = {
        .rect = rect,
        .tex_coords = tex_coords,
    };
    quad_manager_add(&graphics->quad_manager, quad);

    Transform transform = transform_from_xyz(0.0, 0.0, 0.0);
    transform_manager_add(&graphics->transform_manager, transform);
}

void graphics_render(Graphics *graphics)
{
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
        &builder, graphics->wgpu.device, graphics->bind_group_layouts.basic,
        "Transform Bind Group");

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

    WGPURenderPassColorAttachment attachments[] = {
        {
            .view = frame,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .clearValue = {sin(SDL_GetTicks() / 1000.0f), 0.2f, 0.3f, 1.0f},
        },
    };
    WGPURenderPassDescriptor render_pass_desc = {
        .label = "render pass encoder",
        .colorAttachmentCount = 1,
        .colorAttachments = attachments,
    };
    WGPURenderPassEncoder render_pass =
        wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_desc);

    typedef struct
    {
        mat4s camera;
        u32 transform_index;
        u32 texture_index;
    } PushConstants;

    // bind pipeline and buffers
    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.basic);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, transform_bind_group, 0,
                                      0);
    u32 buffer_size = wgpuBufferGetSize(graphics->quad_manager.buffer);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, buffer_size);

    mat4s camera_projection =
        glms_perspective_rh_no(1.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    mat4s camera_transform = glms_look_rh_no(
        (vec3s){.x = 0.0, .y = 0.0, .z = -100.0}, GLMS_ZUP, GLMS_YUP);
    mat4s camera = glms_mat4_mul(camera_projection, camera_transform);
    PushConstants push_constants = {
        .camera = camera,
        .transform_index = 0,
        .texture_index = 0,
    };
    wgpuRenderPassEncoderSetPushConstants(
        render_pass, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0,
        sizeof(PushConstants), &push_constants);
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

    wgpuRenderPipelineRelease(graphics->shaders.basic);
    wgpuBindGroupLayoutRelease(graphics->bind_group_layouts.basic);

    wgpu_resources_free(&graphics->wgpu);
}
