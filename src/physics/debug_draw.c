#include "debug_draw.h"
#include "graphics/graphics.h"
#include "graphics/shaders.h"
#include "utility/common_defines.h"
#include "webgpu.h"
#include "wgpu.h"

typedef struct
{
    Graphics *graphics;
    WGPURenderPassEncoder pass;
    Camera raw_camera;
} DebugCtx;

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
    DebugCtx *ctx = context;

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
    wgpuRenderPassEncoderDraw(
        ctx->pass, VERTICES_PER_QUAD, 1,
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
    draw_maybe_solid_circle(center, size, color, context, 1);
}

void physics_debug_draw(Physics *physics, Graphics *graphics, Camera raw_camera,
                        WGPURenderPassEncoder pass)
{
    DebugCtx ctx = {graphics, pass, .raw_camera = raw_camera};
    b2DebugDraw debug_draw = {.context = &ctx,
                              .DrawCircle = draw_circle,
                              .DrawPoint = draw_point,
                              .DrawSolidCircle = draw_solid_circle,
                              .drawContacts = true};

    b2World_Draw(physics->world, &debug_draw);
}
