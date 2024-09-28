#include "ui_sprite.h"
#include "core_types.h"
#include "graphics/quad_manager.h"
#include "graphics/tex_manager.h"
#include "sensible_nums.h"

void ui_sprite_init(UiSprite *sprite, TextureEntry *texture,
                    TransformEntry transform, QuadEntry quad, f32 opacity)
{
    sprite->texture = texture;
    sprite->transform = transform;
    sprite->quad = quad;
    sprite->opacity = opacity;
}

void ui_sprite_free(UiSprite *sprite, Graphics *graphics)
{
    transform_manager_remove(&graphics->transform_manager, sprite->transform);
    quad_manager_remove(&graphics->quad_manager, sprite->quad);
    texture_manager_unload(&graphics->texture_manager, sprite->texture);
}

void ui_sprite_render(UiSprite *sprite, mat4s camera,
                      WGPURenderPassEncoder pass)
{

    UiSpritePushConstants push_constants = {
        .camera = camera,
        .texture_index = sprite->texture->index,
        .transform_index = sprite->transform,
        .opacity = sprite->opacity,
    };

    wgpuRenderPassEncoderSetPushConstants(
        pass, WGPUShaderStage_Fragment | WGPUShaderStage_Vertex, 0,
        sizeof(SpritePushConstants), &push_constants);
    wgpuRenderPassEncoderDrawIndexed(pass, VERTICES_PER_QUAD, 1, 0,
                                     QUAD_ENTRY_TO_VERTEX_INDEX(sprite->quad),
                                     0);
}
