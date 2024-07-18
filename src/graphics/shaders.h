#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"
#include "wgpu_resources.h"

typedef struct Shaders
{
    WGPURenderPipeline basic;
} Shaders;

void shaders_init(Shaders *shaders, WGPUResources *resources);
