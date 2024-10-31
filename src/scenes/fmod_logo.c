#include "fmod_logo.h"
#include "graphics/graphics.h"
#include "graphics/quad_manager.h"
#include "graphics/tex_manager.h"
#include "graphics/transform_manager.h"
#include "graphics/ui_sprite.h"
#include "utility/common_defines.h"
#include "scenes/title.h"

typedef struct
{
    TextureEntry *logo_texture;
    UiSprite logo_sprite;
    LayerEntry logo_layer;
    f32 time;
} FmodLogoScene;

void fmod_logo_scene_init(Scene **scene_data, Resources *resources,
                          void *extra_args)
{
    (void)extra_args;

    FmodLogoScene *scene =
        (FmodLogoScene *)(*scene_data = malloc(sizeof(FmodLogoScene)));
    scene->time = 0;

    scene->logo_texture = texture_manager_load(
        &resources->graphics->texture_manager, "assets/textures/fmod_logo.png",
        &resources->graphics->wgpu);
    WGPUTexture wgpu_tex = texture_manager_get_texture(
        &resources->graphics->texture_manager, scene->logo_texture);
    u32 width = wgpuTextureGetWidth(wgpu_tex),
        height = wgpuTextureGetHeight(wgpu_tex);

    Quad logo_quad = quad_init(rect_from_size((vec2s){.x = width, .y = height}),
                               RECT_UNIT_TEX_COORDS);

    TransformEntry transform = transform_manager_add(
        &resources->graphics->transform_manager,
        transform_from_xyz((UI_VIEW_WIDTH / 2) - (width / 2),
                           (UI_VIEW_HEIGHT / 2) - (height / 2), 0));

    ui_sprite_init(
        &scene->logo_sprite, scene->logo_texture, transform,
        quad_manager_add(&resources->graphics->quad_manager, logo_quad), 0.0f);
    scene->logo_layer = layer_add(&resources->graphics->ui_layers.background,
                                  &scene->logo_sprite);

    if (resources->debug_mode)
        scene_change(TITLE_SCENE, resources, NULL);
}

void fmod_logo_scene_update(Scene *scene_data, Resources *resources)
{
    FmodLogoScene *scene = (FmodLogoScene *)scene_data;
    f32 delta = duration_as_secs(resources->time.current.delta);

    if (scene->logo_sprite.opacity < 1.0f && scene->time < 1.0f)
    {
        scene->logo_sprite.opacity += delta * 2;
    }
    else if (scene->time > 1.0f && scene->logo_sprite.opacity > 0.0f)
    {
        scene->logo_sprite.opacity -= delta * 2;
    }
    else if (scene->logo_sprite.opacity <= 0.0f)
    {
        scene_change(TITLE_SCENE, resources, NULL);
        return;
    }

    scene->time += duration_as_secs(resources->time.current.delta);
}

void fmod_logo_scene_free(Scene *scene_data, Resources *resources)
{
    FmodLogoScene *scene = (FmodLogoScene *)scene_data;
    ui_sprite_free(&scene->logo_sprite, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.background, scene->logo_layer);
    free(scene_data);
}

const SceneInterface FMOD_LOGO_SCENE = {.init = fmod_logo_scene_init,
                                        .update = fmod_logo_scene_update,
                                        .free = fmod_logo_scene_free};
