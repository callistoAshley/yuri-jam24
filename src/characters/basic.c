#include "basic.h"
#include "scenes/map.h"

typedef struct
{
    char *sprite_name;
    char *event_name;
    
    Transform transform;
    TransformEntry transform_entry;
    TextureEntry *texture;
    Sprite sprite;
    Quad quad;
    LayerEntry layer_entry;
} BasicCharState;

void basic_char_init(void **out, Resources *resources, MapScene *map_scene, Rect rect, HashMap *metadata, void *extra_args)
{
    (void)map_scene;
    (void)extra_args;

    BasicCharState *state = (*out = malloc(sizeof(BasicCharState)));

    state->transform = transform_from_xyz(rect.min.x, rect.min.y, 0);
    state->transform_entry = transform_manager_add(&resources->graphics->transform_manager, state->transform);

    state->sprite_name = hashmap_get(metadata, "sprite");
    if (state->sprite_name)
    {
        state->texture = texture_manager_load(&resources->graphics->texture_manager, state->sprite_name, &resources->graphics->wgpu);
        WGPUTexture wgpu_tex = texture_manager_get_texture(&resources->graphics->texture_manager, state->texture);
        u32 width = wgpuTextureGetWidth(wgpu_tex), height = wgpuTextureGetHeight(wgpu_tex);

        state->quad = quad_init(
            rect_from_size((vec2s){.x = width, .y = height}),
            RECT_UNIT_TEX_COORDS
        );

        sprite_init(&state->sprite, state->texture, state->transform_entry, quad_manager_add(&resources->graphics->quad_manager, state->quad));
        state->layer_entry = layer_add(&resources->graphics->sprite_layers.middle, &state->sprite);
    }

    state->event_name = hashmap_get(metadata, "event");
}

void basic_char_update(void *self, Resources *resources, MapScene *map_scene)
{
    (void)self;
    (void)resources;
    (void)map_scene;
}

void basic_char_free(void *self, Resources *resources, MapScene *map_scene)
{
    free(self);
    (void)resources;
    (void)map_scene;
}

const CharacterInterface BASIC_CHARACTER_INTERFACE = 
{
    .name = "basic",
    .init_fn = basic_char_init,
    .update_fn = basic_char_update,
    .free_fn = basic_char_free
};
