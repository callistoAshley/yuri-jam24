#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"

struct Graphics;

typedef struct Shaders
{
    WGPURenderPipeline basic;
} Shaders;

void shaders_init(struct Graphics *graphics);
