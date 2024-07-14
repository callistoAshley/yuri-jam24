#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"

typedef struct
{
    WGPURenderPipeline basic;
} Shaders;

void shaders_init(Shaders *shaders);
