#include "animation.h"
#include "animation/definition.h"
#include "core_types.h"
#include "graphics/tex_manager.h"
#include "time/time.h"
#include "webgpu.h"

void animation_init(Animation *animation, AnimationType type)
{
    animation->def = ANIMATIONS[type];
    animation_reset(animation);
}

void animation_reset(Animation *animation)
{
    animation->current_frame = 0;
    animation->wait_time = animation->def->frames[0].frame_time;

    animation->needs_apply = true;
    animation->finished = false;
}

void animation_update(Animation *animation, Time time)
{
    if (animation->finished)
    {
        return;
    }

    animation->needs_apply = false;

    f32 delta = time_delta_seconds(time);
    animation->wait_time -= delta;

    if (animation->wait_time <= 0.0)
    {
        animation->current_frame++;
        if (animation->current_frame >= animation->def->frame_count)
        {
            if (animation->def->looping)
                animation->current_frame = 0;
            else
                animation->finished = true;
        }

        Frame frame = animation->def->frames[animation->current_frame];
        animation->wait_time = frame.frame_time;
        animation->needs_apply = true;
    }
}

void animation_apply(Animation *animation, Graphics *graphics, Quad *quad,
                     Sprite *sprite)
{
    if (!animation->needs_apply)
        return;

    WGPUTexture texture = texture_manager_get_texture(
        &graphics->texture_manager, sprite->texture);
    u32 texture_width = wgpuTextureGetWidth(texture);
    u32 texture_height = wgpuTextureGetHeight(texture);

    quad->tex_coords = tex_coords_for(animation->def, animation->current_frame,
                                      texture_width, texture_height);
    quad_manager_update(&graphics->quad_manager, sprite->quad, *quad);
}

void animation_apply_caster(Animation *animation, ShadowCaster *caster)
{
    if (!animation->needs_apply)
        return;

    Frame frame = animation->def->frames[animation->current_frame];
    caster->cell = frame.cell;
}

void animation_free(Animation *animation) { (void)animation; }
