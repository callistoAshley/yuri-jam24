#include "graphics.h"
#include "sensible_nums.h"
#include "webgpu.h"
#include <assert.h>

WGPUTexture texture_from_surface(SDL_Surface *surface, WGPUTextureUsage usage,
                                 WGPUResources *wgpu)
{
    WGPUTextureFormat view_formats[] = {WGPUTextureFormat_RGBA8Unorm,
                                        WGPUTextureFormat_RGBA8UnormSrgb};
    WGPUTextureDescriptor descriptor = {
        .size =
            (WGPUExtent3D){
                .width = surface->w,
                .height = surface->h,
                .depthOrArrayLayers = 1,
            },
        .mipLevelCount = 1,
        .sampleCount = 1,
        .dimension = WGPUTextureDimension_2D,
        .format = WGPUTextureFormat_RGBA8UnormSrgb,
        .usage = usage | WGPUTextureUsage_CopyDst,
        .viewFormats = view_formats,
        .viewFormatCount = 2,
    };
    WGPUTexture texture = wgpuDeviceCreateTexture(wgpu->device, &descriptor);

    write_surface_to_texture(surface, texture, wgpu);

    return texture;
}

void write_surface_to_texture(SDL_Surface *surface, WGPUTexture texture,
                              WGPUResources *wgpu)
{
    write_surface_to_texture_at(0, 0, surface, texture, wgpu);
}

void write_surface_to_texture_at(u32 x, u32 y, SDL_Surface *surface,
                                 WGPUTexture texture, WGPUResources *wgpu)
{
    SDL_Surface *converted;
    if (surface->format != SDL_PIXELFORMAT_RGBA32)
        converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    else
        converted = surface;

    WGPUTextureFormat format = wgpuTextureGetFormat(texture);
    assert(format == WGPUTextureFormat_RGBA8UnormSrgb);

    WGPUImageCopyTexture copy_texture = {
        .texture = texture,
        .mipLevel = 0,
        .origin = {x, y, 0},
        .aspect = WGPUTextureAspect_All,
    };
    WGPUExtent3D write_size = {
        .width = surface->w,
        .height = surface->h,
        .depthOrArrayLayers = 1,
    };
    WGPUTextureDataLayout layout = {
        .offset = 0,
        .bytesPerRow = 4 * surface->w,
        .rowsPerImage = surface->h,
    };
    wgpuQueueWriteTexture(wgpu->queue, &copy_texture, converted->pixels,
                          4 * surface->w * surface->h, &layout, &write_size);

    if (converted != surface)
        SDL_DestroySurface(converted);
}

WGPUTexture blank_texture(u32 w, u32 h, WGPUTextureUsage usage,
                          WGPUResources *wgpu)
{
    WGPUTextureFormat view_formats[] = {WGPUTextureFormat_RGBA8Unorm,
                                        WGPUTextureFormat_RGBA8UnormSrgb};
    WGPUTextureDescriptor descriptor = {
        .size =
            (WGPUExtent3D){
                .width = w,
                .height = h,
                .depthOrArrayLayers = 1,
            },
        .mipLevelCount = 1,
        .sampleCount = 1,
        .dimension = WGPUTextureDimension_2D,
        .format = WGPUTextureFormat_RGBA8UnormSrgb,
        .usage = usage | WGPUTextureUsage_CopyDst,
        .viewFormats = view_formats,
        .viewFormatCount = 2,
    };
    WGPUTexture texture = wgpuDeviceCreateTexture(wgpu->device, &descriptor);

    return texture;
}
