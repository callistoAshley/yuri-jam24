#pragma once

#include <wgpu.h>
#include <SDL3/SDL.h>

typedef struct
{
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUSurfaceConfiguration surface_config;
} WGPUResources;

void wgpu_resources_init(WGPUResources *resources, SDL_Window *window);
void wgpu_resources_free(WGPUResources *resources);
