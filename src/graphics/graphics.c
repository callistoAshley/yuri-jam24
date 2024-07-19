#include <stdio.h>
#include <wgpu.h>

#include "graphics.h"
#include "cglm/struct/vec2.h"
#include "core_types.h"
#include "graphics/quad_manager.h"
#include "imgui-wgpu.h"
#include "utility/macros.h"

QuadEntry entry;

void graphics_init(Graphics *graphics, SDL_Window *window)
{
    wgpu_resources_init(&graphics->wgpu, window);
    shaders_init(&graphics->shaders, &graphics->wgpu);
    quad_manager_init(&graphics->quad_manager, &graphics->wgpu);

    Rect rect = rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 100, .y = 100});
    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
    Quad quad = {
        .rect = rect,
        .tex_coords = tex_coords,
    };
    entry = quad_manager_add(&graphics->quad_manager, quad);
}

void graphics_render(Graphics *graphics)
{
    quad_manager_upload_dirty(&graphics->quad_manager, &graphics->wgpu);

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

    u32 buffer_size = wgpuBufferGetSize(graphics->quad_manager.buffer);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, buffer_size);
    wgpuRenderPassEncoderDraw(render_pass, 6, 1, 0, 0);

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
    wgpuRenderPipelineRelease(graphics->shaders.basic);
    wgpu_resources_free(&graphics->wgpu);
}
