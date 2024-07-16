#include <stdio.h>
#include <wgpu.h>
#include <SDL3/SDL.h>

#include "graphics.h"
#include "graphics/shaders.h"
#include "utility/macros.h"

static void logCallback(WGPULogLevel level, const char *message, void *userdata)
{
    (void)userdata;
    (void)level;
    printf("(WGPU) %s\n", message);
}

static void handle_request_adapter(WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter, char const *message,
                                   void *userdata)
{
    Graphics *graphics = userdata;
    if (status == WGPURequestAdapterStatus_Success)
    {
        graphics->adapter = adapter;
    }
    else
    {
        printf("request_adapter status: %#.8x message=%s\n", status, message);
    }
}
static void handle_request_device(WGPURequestDeviceStatus status,
                                  WGPUDevice device, char const *message,
                                  void *userdata)
{
    Graphics *graphics = userdata;
    if (status == WGPURequestDeviceStatus_Success)
    {
        graphics->device = device;
    }
    else
    {
        printf("request_adapter status: %#.8x message=%s\n", status, message);
    }
}

const vec2 vertices[] = {
    {0.0f, 0.5f},
    {-0.5f, -0.5f},
    {0.5f, -0.5f},
};
WGPUBuffer vertex_buffer;

#define INVALID_VALUE 0xDEADCAFE
#define INVALID_POINTER (void *)INVALID_VALUE
void graphics_init(Graphics *graphics, SDL_Window *window)
{
    wgpuSetLogCallback(logCallback, NULL);
    wgpuSetLogLevel(WGPULogLevel_Info);

    const WGPUInstanceDescriptor instance_desc = {0};
    graphics->instance = wgpuCreateInstance(&instance_desc);
    PTR_ERRCHK(graphics->instance, "failed to create WGPU instance");

    // other platforms? what are those
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    void *display = SDL_GetProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                                    INVALID_POINTER);
    uint64_t xwindow = SDL_GetNumberProperty(
        props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, INVALID_VALUE);

    SDL_ERRCHK(display == INVALID_POINTER, "display is null");
    SDL_ERRCHK(xwindow == INVALID_VALUE, "xwindow is null");

    WGPUSurfaceDescriptorFromXlibWindow raw_surface_desc = {
        .chain = {.sType = WGPUSType_SurfaceDescriptorFromXlibWindow,
                  .next = NULL},
        .display = display,
        .window = xwindow,
    };
    WGPUSurfaceDescriptor surface_desc = {
        .nextInChain = (const WGPUChainedStruct *)&raw_surface_desc,
    };
    graphics->surface =
        wgpuInstanceCreateSurface(graphics->instance, &surface_desc);
    PTR_ERRCHK(graphics->surface, "failed to create WGPU surface");

    WGPURequestAdapterOptions adapter_options = {
        .nextInChain = NULL,
        .compatibleSurface = graphics->surface,
        .powerPreference = WGPUPowerPreference_HighPerformance,
    };
    // handle_request_adapter is called asynchronously, and is expected to set
    // graphics->adapter
    wgpuInstanceRequestAdapter(graphics->instance, &adapter_options,
                               handle_request_adapter, graphics);
    PTR_ERRCHK(graphics->adapter, "failed to request WGPU adapter");

    // same as above
    wgpuAdapterRequestDevice(graphics->adapter, NULL, handle_request_device,
                             graphics);
    PTR_ERRCHK(graphics->device, "failed to request WGPU device");

    graphics->queue = wgpuDeviceGetQueue(graphics->device);
    PTR_ERRCHK(graphics->queue, "failed to get WGPU queue");

    WGPUSurfaceCapabilities surface_caps;
    wgpuSurfaceGetCapabilities(graphics->surface, graphics->adapter,
                               &surface_caps);

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    graphics->surface_config = (WGPUSurfaceConfiguration){
        .device = graphics->device,
        .usage = WGPUTextureUsage_RenderAttachment,
        .format = surface_caps.formats[0],
        .presentMode = WGPUPresentMode_Fifo,
        .alphaMode = surface_caps.alphaModes[0],
        .width = window_width,
        .height = window_height,
    };
    wgpuSurfaceConfigure(graphics->surface, &graphics->surface_config);

    wgpuSurfaceCapabilitiesFreeMembers(surface_caps);

    shaders_init(graphics);

    WGPUBufferDescriptor descriptor = {
        .label = "vertex buffer",
        .size = sizeof(vertices),
        .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
    };
    vertex_buffer = wgpuDeviceCreateBuffer(graphics->device, &descriptor);
    PTR_ERRCHK(vertex_buffer, "failed to create vertex buffer");
    wgpuQueueWriteBuffer(graphics->queue, vertex_buffer, 0, vertices,
                         sizeof(vertices));
}

void graphics_render(Graphics *graphics, Player *player)
{
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(graphics->surface, &surface_texture);

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
        wgpuSurfaceConfigure(graphics->surface, &graphics->surface_config);

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
        wgpuDeviceCreateCommandEncoder(graphics->device, NULL);

    WGPURenderPassColorAttachment attachments[] = {
        {
            .view = frame,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .clearValue = {0.1f, 0.2f, 0.3f, 1.0f},
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
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer, 0,
                                         sizeof(vertices));
    wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(render_pass);

    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, NULL);
    wgpuQueueSubmit(graphics->queue, 1, &command_buffer);
    wgpuSurfacePresent(graphics->surface);

    wgpuCommandBufferRelease(command_buffer);
    wgpuRenderPassEncoderRelease(render_pass);
    wgpuCommandEncoderRelease(command_encoder);
    wgpuTextureViewRelease(frame);
    wgpuTextureRelease(surface_texture.texture);
}

void graphics_free(Graphics *graphics)
{
    wgpuRenderPipelineRelease(graphics->shaders.basic);

    wgpuSurfaceRelease(graphics->surface);
    wgpuQueueRelease(graphics->queue);
    wgpuDeviceRelease(graphics->device);
    wgpuAdapterRelease(graphics->adapter);
    wgpuInstanceRelease(graphics->instance);
}
