#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"
#include "wgpu_resources.h"

typedef struct
{
    WGPUBindGroupLayout sprite;
    WGPUBindGroupLayout lighting;
    WGPUBindGroupLayout tilemap;
    WGPUBindGroupLayout hdr_tonemap;
    WGPUBindGroupLayout shadowmap;
} BindGroupLayouts;

void bind_group_layouts_init(BindGroupLayouts *layouts,
                             WGPUResources *resources);
