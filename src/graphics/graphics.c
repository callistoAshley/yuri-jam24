#include <stdio.h>
#include <wgpu.h>

#include "graphics.h"
#include "core_types.h"
#include "binding_helper.h"
#include "graphics/quad_manager.h"
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

    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);

    graphics->transform_bind_group = bind_group_build(
        &builder, graphics->wgpu.device, graphics->bind_group_layouts.transform,
        "Transform Bind Group");

    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
    Rect rect = rect_from_min_size((vec2s){.x = 0., .y = 0.},
                                   (vec2s){.x = 32, .y = 32});
    Quad quad = {
        .rect = rect,
        .tex_coords = tex_coords,
    };
    quad_manager_add(&graphics->quad_manager, quad);

    Transform transform = transform_from_xyz(32.0, 32.0, 0.0);
    transform_manager_add(&graphics->transform_manager, transform);
}

void graphics_render(Graphics *graphics)
{
    Transform transform =
        transform_from_xyz(fmod(SDL_GetTicks() / 10.0, 640.0), 32.0, 0.0);
    transform_manager_update(&graphics->transform_manager, 0, transform);

    quad_manager_upload_dirty(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_upload_dirty(&graphics->transform_manager,
                                   &graphics->wgpu);

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

    // bind pipeline and buffers
    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.basic);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0,
                                      graphics->transform_bind_group, 0, 0);
    u32 buffer_size = wgpuBufferGetSize(graphics->quad_manager.buffer);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, buffer_size);

    u32 transform_index = 0;
    wgpuRenderPassEncoderSetPushConstants(render_pass, WGPUShaderStage_Vertex,
                                          0, sizeof(u32), &transform_index);
    wgpuRenderPassEncoderDraw(render_pass, VERTICES_PER_QUAD, 1,
                              QUAD_ENTRY_TO_VERTEX_INDEX(0), 0);

    ImGui_ImplWGPU_RenderDrawData(igGetDrawData(), render_pass);

    wgpuRenderPassEncoderEnd(render_pass);

    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, NULL);
    wgpuQueueSubmit(graphics->wgpu.queue, 1, &command_buffer);
    wgpuSurfacePresent(graphics->wgpu.surface);

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
    wgpuBindGroupRelease(graphics->transform_bind_group);

    wgpuRenderPipelineRelease(graphics->shaders.basic);
    wgpuBindGroupLayoutRelease(graphics->bind_group_layouts.transform);

    wgpu_resources_free(&graphics->wgpu);
}
