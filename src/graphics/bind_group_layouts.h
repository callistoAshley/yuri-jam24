#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"
#include "wgpu_resources.h"

typedef struct
{
    WGPUBindGroupLayout object;
    WGPUBindGroupLayout lighting;
    WGPUBindGroupLayout tilemap;
} BindGroupLayouts;

void bind_group_layouts_init(BindGroupLayouts *layouts,
                             WGPUResources *resources);
