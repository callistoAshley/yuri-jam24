#pragma once

#include "graphics/graphics.h"
#include "physics.h"

// max amount of vertices we can draw
#define MAX_VERTEX_COUNT 1024
#define INDEX_COUNT (MAX_VERTEX_COUNT - 2) * 3

typedef struct
{
    Graphics *graphics;
    WGPURenderPassEncoder pass;
    Camera raw_camera;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;

    // where we can write to in the vertex buffer
    u32 vertex_index;
    u32 index_index;
} Box2DDebugCtx;

void physics_debug_draw_init(Box2DDebugCtx *ctx, Graphics *graphics,
                             Camera raw_camera);
void physics_debug_draw(Box2DDebugCtx *ctx, Physics *physics,
                        WGPURenderPassEncoder pass);
// call this after finished with the renderpass!
void physics_debug_draw_free(Box2DDebugCtx *ctx);
