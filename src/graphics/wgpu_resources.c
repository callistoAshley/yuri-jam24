#include <stdio.h>
#include <wgpu.h>
#include <webgpu.h>
#include <SDL3/SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "wgpu_resources.h"
#include "utility/macros.h"

static void logCallback(WGPULogLevel level, const char *message, void *userdata)
{
    (void)userdata;
    (void)level;
    if (level == WGPULogLevel_Error)
        printf("(WGPU) ERROR: %s\n", message);
    else if (level == WGPULogLevel_Warn)
        printf("(WGPU) WARNING: %s\n", message);
}

static void handle_request_adapter(WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter, char const *message,
                                   void *userdata)
{
    WGPUResources *resources = userdata;
    if (status == WGPURequestAdapterStatus_Success)
    {
        resources->adapter = adapter;
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
    WGPUResources *resources = userdata;
    if (status == WGPURequestDeviceStatus_Success)
    {
        resources->device = device;
    }
    else
    {
        printf("request_adapter status: %#.8x message=%s\n", status, message);
    }
}

#define INVALID_VALUE 0xDEADCAFE
#define INVALID_POINTER (void *)INVALID_VALUE

void wgpu_resources_init(WGPUResources *resources, SDL_Window *window)
{
    wgpuSetLogCallback(logCallback, NULL);
    wgpuSetLogLevel(WGPULogLevel_Info);

    printf("WGPU Version: %d\n", wgpuGetVersion());

    WGPUInstanceExtras instance_extras = {
        .chain = {.sType = (WGPUSType)WGPUSType_InstanceExtras},
        // we can only support these backends because they're the only ones with
        // bindless
        .backends = WGPUBackendType_Vulkan | WGPUBackendType_Metal |
                    WGPUBackendType_D3D12,
        // TODO disable on release
        .flags = WGPUInstanceFlag_Debug | WGPUInstanceBackend_Vulkan};
    WGPUInstanceDescriptor instance_desc = {
        .nextInChain = (WGPUChainedStruct *)&instance_extras};
    resources->instance = wgpuCreateInstance(&instance_desc);
    PTR_ERRCHK(resources->instance, "failed to create WGPU instance");

    // other platforms? what are those
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
#ifdef __linux__
    void *display = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, INVALID_POINTER);
    uint64_t xwindow = SDL_GetNumberProperty(
        props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, INVALID_VALUE);

    SDL_ERRCHK(display == INVALID_POINTER, "display is null");
    SDL_ERRCHK(xwindow == INVALID_VALUE, "xwindow is null");

    WGPUSurfaceDescriptorFromXlibWindow raw_surface_desc = {
        .chain = {.sType = WGPUSType_SurfaceDescriptorFromXlibWindow},
        .display = display,
        .window = xwindow,
    };
#elif _WIN32
    void *hwnd = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, INVALID_POINTER);
    SDL_ERRCHK(hwnd == INVALID_POINTER, "hwnd is null");

    WGPUSurfaceDescriptorFromWindowsHWND raw_surface_desc = {
        .chain = {.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND},
        .hinstance = GetModuleHandle(NULL),
        .hwnd = hwnd,
    };
#else
#error "Unsupported platform"
#endif
    WGPUSurfaceDescriptor surface_desc = {
        .nextInChain = (const WGPUChainedStruct *)&raw_surface_desc,
    };
    resources->surface =
        wgpuInstanceCreateSurface(resources->instance, &surface_desc);
    PTR_ERRCHK(resources->surface, "failed to create WGPU surface");

    WGPURequestAdapterOptions adapter_options = {
        .compatibleSurface = resources->surface,
        .powerPreference = WGPUPowerPreference_HighPerformance,
    };
    // handle_request_adapter is called asynchronously, and is expected to set
    // graphics->adapter
    wgpuInstanceRequestAdapter(resources->instance, &adapter_options,
                               handle_request_adapter, resources);
    PTR_ERRCHK(resources->adapter, "failed to request WGPU adapter");

    // SIGNIFICANTLY reduce the hardware we can support. still runs on my shitty
    // macbook from 2014 though
    WGPUFeatureName features[] = {
        // the bindless friends :3
        (WGPUFeatureName)WGPUNativeFeature_MultiDrawIndirect,
        // we would need multidraw indirect count, but we're not going to be
        // doing any gpu-based culling so we should be fine..?
        (WGPUFeatureName)WGPUNativeFeature_TextureBindingArray,
        (WGPUFeatureName)WGPUNativeFeature_PartiallyBoundBindingArray,
        // this one is probably not required if we use multi-draw
        (WGPUFeatureName)
            WGPUNativeFeature_SampledTextureAndStorageBufferArrayNonUniformIndexing,
        // any hardware that we target that supports the above should support
        // this, right???
        (WGPUFeatureName)WGPUNativeFeature_PushConstants};

    // this imposes an upper limit of 2048 textures per shader stage, which is a
    // LOT of wiggle room
    // unfortunately because of some oversight in the WGPU API, we have to
    // specify every limit, or none at all. so we have to specify all of them
    // we could get the limits from the adapter, but that means that our code
    // could rely on really high limits (i have a 1080ti) which is obviously not
    // good
    // these defaults are copied from
    // https://docs.rs/wgpu-types/0.20.0/src/wgpu_types/lib.rs.html#1186
    WGPULimits supported_limits = {
        .maxSampledTexturesPerShaderStage = 2048,

        // default values
        .maxTextureDimension1D = 8192,
        .maxTextureDimension2D = 8192,
        .maxTextureDimension3D = 2048,
        .maxTextureArrayLayers = 256,
        .maxBindGroups = 4,
        .maxBindingsPerBindGroup = 1000,
        .maxDynamicUniformBuffersPerPipelineLayout = 8,
        .maxDynamicStorageBuffersPerPipelineLayout = 4,
        // we don't really use much samplers luckily. all we care about
        // is nearest neighbor
        .maxSamplersPerShaderStage = 16,
        .maxStorageBuffersPerShaderStage = 8,
        .maxStorageTexturesPerShaderStage = 4,
        .maxUniformBuffersPerShaderStage = 12,
        .maxUniformBufferBindingSize = 64 << 10,  // 64kb
        .maxStorageBufferBindingSize = 128 << 20, // 128mb
        .maxVertexBuffers = 8,
        .maxBufferSize = 256 << 20, // 256mb
        .maxVertexAttributes = 16,
        .maxVertexBufferArrayStride = 2048,
        // i have never seen hardware where this is not 256
        .minUniformBufferOffsetAlignment = 256,
        .minStorageBufferOffsetAlignment = 256,
        .maxInterStageShaderComponents = 60,
        .maxColorAttachments = 8,
        .maxColorAttachmentBytesPerSample = 32,
        .maxComputeWorkgroupStorageSize = 16384,
        .maxComputeInvocationsPerWorkgroup = 256,
        .maxComputeWorkgroupSizeX = 256,
        .maxComputeWorkgroupSizeY = 256,
        .maxComputeWorkgroupSizeZ = 64,
        .maxComputeWorkgroupsPerDimension = 65535,
    };

    // gotta love c polymorphism
    WGPURequiredLimitsExtras native_limits = {
        .chain = {.sType = (WGPUSType)WGPUSType_RequiredLimitsExtras},
        .limits = {
            .maxNonSamplerBindings = 1000000, // copied from wgpu's default
            .maxPushConstantSize = 128,       // lowest i've seen is 128
        }};

    WGPURequiredLimits limits = {.nextInChain =
                                     (WGPUChainedStruct *)&native_limits,
                                 .limits = supported_limits};

    WGPUDeviceDescriptor device_descriptor = {
        .requiredFeatures = features,
        .requiredFeatureCount = 5,
        .requiredLimits = &limits,
    };
    // same as above
    wgpuAdapterRequestDevice(resources->adapter, &device_descriptor,
                             handle_request_device, resources);
    PTR_ERRCHK(resources->device, "failed to request WGPU device");

    resources->queue = wgpuDeviceGetQueue(resources->device);
    PTR_ERRCHK(resources->queue, "failed to get WGPU queue");

    WGPUSurfaceCapabilities surface_caps;
    wgpuSurfaceGetCapabilities(resources->surface, resources->adapter,
                               &surface_caps);

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    resources->surface_config = (WGPUSurfaceConfiguration){
        .device = resources->device,
        .usage = WGPUTextureUsage_RenderAttachment,
        .format = surface_caps.formats[0],
        .presentMode = WGPUPresentMode_Fifo,
        .alphaMode = surface_caps.alphaModes[0],
        .width = window_width,
        .height = window_height,
    };
    wgpuSurfaceConfigure(resources->surface, &resources->surface_config);

    wgpuSurfaceCapabilitiesFreeMembers(surface_caps);
}

void wgpu_resources_free(WGPUResources *resources)
{
    wgpuSurfaceRelease(resources->surface);
    wgpuQueueRelease(resources->queue);
    wgpuDeviceRelease(resources->device);
    wgpuAdapterRelease(resources->adapter);
    wgpuInstanceRelease(resources->instance);
}
