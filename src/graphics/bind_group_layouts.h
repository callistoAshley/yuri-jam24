#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"
#include "wgpu_resources.h"

typedef struct
{
    WGPUBindGroupLayout basic;
} BindGroupLayouts;

void bing_group_layouts_init(BindGroupLayouts *layouts,
                             WGPUResources *resources);
