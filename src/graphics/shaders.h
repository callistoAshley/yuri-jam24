#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"
#include "wgpu_resources.h"
#include "bind_group_layouts.h"

#include <cglm/struct.h>

typedef struct Shaders
{
    WGPURenderPipeline object;
    WGPURenderPipeline lighting;
} Shaders;

typedef struct
{
    mat4s camera;
    u32 transform_index;
    u32 texture_index;
} ObjectPushConstants;

void shaders_init(Shaders *shaders, BindGroupLayouts *layouts,
                  WGPUResources *resources);
