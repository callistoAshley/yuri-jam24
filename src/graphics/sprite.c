#include "sprite.h"
#include "core_types.h"
#include "graphics/quad_manager.h"

void sprite_init(Sprite *sprite, TextureEntry *texture,
                 TransformEntry transform, QuadEntry quad)
{
    sprite->texture = texture;
    sprite->transform = transform;
    sprite->quad = quad;

    sprite->normal_tex = NULL;
}

void sprite_free(Sprite *sprite, Graphics *graphics)
{
    transform_manager_remove(&graphics->transform_manager, sprite->transform);
    quad_manager_remove(&graphics->quad_manager, sprite->quad);
}

void sprite_render(Sprite *sprite, mat4s camera, WGPURenderPassEncoder pass)
{
    i32 normal_index = -1;
    if (sprite->normal_tex)
        normal_index = sprite->normal_tex->index;

    ObjectPushConstants push_constants = {
        .camera = camera,
        .texture_index = sprite->texture->index,
        .normal_texture_index = normal_index,
        .transform_index = sprite->transform,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(ObjectPushConstants), &push_constants);
    wgpuRenderPassEncoderDrawIndexed(pass, VERTICES_PER_QUAD, 1, 0,
                                     QUAD_ENTRY_TO_VERTEX_INDEX(sprite->quad),
                                     0);
}
