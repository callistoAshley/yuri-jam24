#pragma once

#include <stdint.h>
#include <wgpu.h>
#include "utility/vec.h"

struct Graphics;

typedef struct
{
    WGPUBuffer buffer;
    uint32_t written_len;
    vec unwritten; // vec<Vertex>
} VertexBuffer;

typedef struct
{
    VertexBuffer buffer;
    vec entries; // either occupied, or an index to the next free entry
} QuadManager;

typedef uint32_t QuadEntry;