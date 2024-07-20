#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"
#include "wgpu_resources.h"
#include "bind_group_layouts.h"

typedef struct Shaders
{
    WGPURenderPipeline object;
    WGPURenderPipeline lighting;
} Shaders;

void shaders_init(Shaders *shaders, BindGroupLayouts *layouts,
                  WGPUResources *resources);
