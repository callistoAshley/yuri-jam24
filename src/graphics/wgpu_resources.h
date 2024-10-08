#pragma once

#include "sensible_nums.h"
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
    WGPUSurfaceCapabilities surface_caps;
} WGPUResources;

typedef struct
{
    u32 vertex_count;
    u32 instance_count;
    u32 first_vertex;
    // first instance must be enabled to support this, but
    // we kinda can't do that because it's not exposed in
    // wgpu-native
    u32 first_instance;
} DrawIndirectArgs;

typedef struct
{
    u32 index_count;
    u32 instance_count;
    u32 first_index;
    u32 base_vertex;
    u32 first_instance;
} DrawIndexedIndirectArgs;

void wgpu_resources_init(WGPUResources *resources, SDL_Window *window);
void wgpu_resources_free(WGPUResources *resources);
