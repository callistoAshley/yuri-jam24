#include "debug_draw.h"
#include "graphics/graphics.h"
#include "graphics/shaders.h"
#include "utility/common_defines.h"
#include "webgpu.h"
#include "wgpu.h"

vec3s vec3_from_hex(b2HexColor color)
{
    return (vec3s){
        .x = ((color >> 16) & 0xFF) / 255.0f,
        .y = ((color >> 8) & 0xFF) / 255.0f,
        .z = (color & 0xFF) / 255.0f,
    };
}

void draw_maybe_solid_circle(b2Vec2 center, float radius, b2HexColor color,
                             void *context, bool solid)
{
    Box2DDebugCtx *ctx = context;

    u64 quad_buffer_size =
        wgpuBufferGetSize(ctx->graphics->quad_manager.buffer);

    wgpuRenderPassEncoderSetPipeline(ctx->pass,
                                     ctx->graphics->shaders.box2d_debug.circle);

    float scale =
        (float)ctx->graphics->wgpu.surface_config.width / INTERNAL_SCREEN_WIDTH;

    B2DCirclePushConstants push_constants = {
        .color = vec3_from_hex(color),
        .camera_position = {.x = ctx->raw_camera.x, .y = ctx->raw_camera.y},
        .position = {.x = center.x, .y = -center.y},
        .radius = radius,
        .internal_scale = scale,
        .solid = solid,
    };
    wgpuRenderPassEncoderSetPushConstants(
        ctx->pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(B2DCirclePushConstants), &push_constants);

    wgpuRenderPassEncoderSetVertexBuffer(
        ctx->pass, 0, ctx->graphics->quad_manager.buffer, 0, quad_buffer_size);
    wgpuRenderPassEncoderSetIndexBuffer(
        ctx->pass, ctx->graphics->quad_manager.index_buffer,
        WGPUIndexFormat_Uint16, 0, 16);
    wgpuRenderPassEncoderDrawIndexed(
        ctx->pass, VERTICES_PER_QUAD, 1, 0,
        QUAD_ENTRY_TO_VERTEX_INDEX(graphics_screen_quad_entry()), 0);
}

// FIXME we should be drawing a line to indicate the rotation of the circle
void draw_solid_circle(b2Transform transform, float radius, b2HexColor color,
                       void *context)
{
    draw_maybe_solid_circle(transform.p, radius, color, context, 1);
}

void draw_circle(b2Vec2 center, float radius, b2HexColor color, void *context)
{
    draw_maybe_solid_circle(center, radius, color, context, 0);
}

void draw_point(b2Vec2 center, float size, b2HexColor color, void *context)
{
    Box2DDebugCtx *ctx = context;
    float scale =
        (float)ctx->graphics->wgpu.surface_config.width / INTERNAL_SCREEN_WIDTH;
    draw_maybe_solid_circle(center, size / PX_PER_M / scale, color, context, 1);
}

void draw_maybe_solid_polygon(b2Transform transform, const b2Vec2 *vertices,
                              i32 vertex_count, b2HexColor color, void *context,
                              bool solid)
{
    Box2DDebugCtx *ctx = context;
    // this is a bit of a nightmare to do...
    // box2d gives us a CCW list of vertices, but we need to draw them as a
    // triangles!
    // easiest way to do this is to make a fan-triangulated index buffer.
    u32 vertex_buffer_size = sizeof(b2Vec2) * vertex_count;
    wgpuQueueWriteBuffer(ctx->graphics->wgpu.queue, ctx->vertex_buffer,
                         ctx->vertex_index, vertices, vertex_buffer_size);

    // construct the index buffer
    u32 index_count = (vertex_count - 2) * 3;
    u32 index_buffer_size = sizeof(u32) * index_count;
    u32 *indices = malloc(index_buffer_size);
    for (i32 i = 0; i < vertex_count - 2; i++)
    {
        indices[i * 3] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    wgpuQueueWriteBuffer(ctx->graphics->wgpu.queue, ctx->index_buffer,
                         ctx->index_index, indices,
                         sizeof(u32) * (vertex_count - 2) * 3);
    free(indices);

    wgpuRenderPassEncoderSetPipeline(
        ctx->pass, ctx->graphics->shaders.box2d_debug.polygon);

    float scale =
        (float)ctx->graphics->wgpu.surface_config.width / INTERNAL_SCREEN_WIDTH;

    // FIXME handle rotation
    B2DDrawPolygonPushConstants push_constants = {
        .color = vec3_from_hex(color),
        .camera_position = {.x = ctx->raw_camera.x, .y = ctx->raw_camera.y},
        .position = {.x = transform.p.x, .y = -transform.p.y},
        .internal_scale = scale,
        .solid = solid,
    };

    wgpuRenderPassEncoderSetPushConstants(
        ctx->pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(B2DDrawPolygonPushConstants), &push_constants);

    wgpuRenderPassEncoderSetVertexBuffer(ctx->pass, 0, ctx->vertex_buffer,
                                         ctx->vertex_index, vertex_buffer_size);
    wgpuRenderPassEncoderSetIndexBuffer(ctx->pass, ctx->index_buffer,
                                        WGPUIndexFormat_Uint32,
                                        ctx->index_index, index_buffer_size);

    wgpuRenderPassEncoderDrawIndexed(ctx->pass, index_count, 1, 0, 0, 0);

    ctx->vertex_index += vertex_buffer_size;
    ctx->index_index += index_buffer_size;
}

void draw_polygon(const b2Vec2 *vertices, i32 vertex_count, b2HexColor color,
                  void *context)
{
    // ??
    draw_maybe_solid_polygon(b2Transform_identity, vertices, vertex_count,
                             color, context, 0);
}

void draw_solid_polygon(b2Transform transform, const b2Vec2 *vertices,
                        i32 vertex_count, f32 radius, b2HexColor color,
                        void *context)
{
    (void)radius;
    draw_maybe_solid_polygon(transform, vertices, vertex_count, color, context,
                             1);
}

void physics_debug_draw_init(Box2DDebugCtx *ctx, Graphics *graphics,
                             Camera raw_camera)
{
    u32 vertex_buffer_size = sizeof(b2Vec2) * MAX_VERTEX_COUNT;
    WGPUBufferDescriptor vertex_buffer_desc = {
        .label = "box2d debug vertex buffer",
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        .size = vertex_buffer_size,
    };
    ctx->vertex_buffer =
        wgpuDeviceCreateBuffer(graphics->wgpu.device, &vertex_buffer_desc);

    u32 index_buffer_size = sizeof(u32) * INDEX_COUNT;
    WGPUBufferDescriptor index_buffer_desc = {
        .label = "box2d debug index buffer",
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
        .size = index_buffer_size,
    };
    ctx->index_buffer =
        wgpuDeviceCreateBuffer(graphics->wgpu.device, &index_buffer_desc);

    ctx->vertex_index = 0;
    ctx->index_index = 0;

    ctx->graphics = graphics;
    ctx->raw_camera = raw_camera;
}

void physics_debug_draw(Box2DDebugCtx *ctx, Physics *physics,
                        WGPURenderPassEncoder pass)
{
    ctx->pass = pass;
    b2DebugDraw debug_draw = {.context = ctx,
                              .DrawCircle = draw_circle,
                              .DrawPoint = draw_point,
                              .DrawSolidCircle = draw_solid_circle,
                              .DrawPolygon = draw_polygon,
                              .DrawSolidPolygon = draw_solid_polygon,
                              .drawShapes = true,
                              .drawContacts = true};

    b2World_Draw(physics->world, &debug_draw);
}

void physics_debug_draw_free(Box2DDebugCtx *ctx)
{
    wgpuBufferRelease(ctx->vertex_buffer);
    wgpuBufferRelease(ctx->index_buffer);
}
