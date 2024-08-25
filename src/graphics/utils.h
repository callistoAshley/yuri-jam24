#pragma once

#include <SDL3/SDL.h>
#include <wgpu.h>
#include "wgpu_resources.h"

WGPUTexture texture_from_surface(SDL_Surface *surface, WGPUTextureUsage usage,
                                 WGPUResources *wgpu);
void write_surface_to_texture(SDL_Surface *surface, WGPUTexture texture,
                              WGPUResources *wgpu);
