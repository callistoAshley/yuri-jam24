#include "basic.h"
#include "scenes/map.h"
#include "utility/log.h"
#include "input/input.h"
#include <string.h>

typedef struct
{
    char sprite_name[256];
    char event_name[256];

    Transform transform;
    TransformEntry transform_entry;
    TextureEntry *texture;
    Sprite sprite;
    Quad quad;
    LayerEntry layer_entry;

    Rect rect;
} BasicCharState;

void basic_char_init(void **out, Resources *resources, MapScene *map_scene,
                     Rect rect, HashMap *metadata, void *extra_args)
{
    (void)map_scene;
    (void)extra_args;

    BasicCharState *state = (*out = malloc(sizeof(BasicCharState)));
    state->rect = rect;
    memset(state->sprite_name, 0, sizeof(state->sprite_name));
    memset(state->event_name, 0, sizeof(state->event_name));

    state->transform = transform_from_xyz(rect.min.x, rect.min.y, 0);
    state->transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, state->transform);

    if (hashmap_get(metadata, "sprite"))
    {
        strncpy(state->sprite_name, hashmap_get(metadata, "sprite"),
                sizeof(state->sprite_name));
        state->texture = texture_manager_load(
            &resources->graphics->texture_manager, state->sprite_name,
            &resources->graphics->wgpu);
        WGPUTexture wgpu_tex = texture_manager_get_texture(
            &resources->graphics->texture_manager, state->texture);
        u32 width = wgpuTextureGetWidth(wgpu_tex),
            height = wgpuTextureGetHeight(wgpu_tex);

        state->quad =
            quad_init(rect_from_size((vec2s){.x = width, .y = height}),
                      RECT_UNIT_TEX_COORDS);

        sprite_init(
            &state->sprite, state->texture, state->transform_entry,
            quad_manager_add(&resources->graphics->quad_manager, state->quad));
        state->layer_entry = layer_add(
            &resources->graphics->sprite_layers.middle, &state->sprite);
    }

    if (hashmap_get(metadata, "event"))
        strncpy(state->event_name, hashmap_get(metadata, "event"),
                sizeof(state->event_name));
}

void basic_char_update(void *self, Resources *resources, MapScene *map_scene)
{
    BasicCharState *state = self;

    Rect player_rect =
        rect_from_min_size((vec2s){.x = map_scene->player.transform.position.x,
                                   .y = map_scene->player.transform.position.y},
                           (vec2s){.x = PLAYER_W, .y = PLAYER_H});

    // if (rect_contains_other(player_rect, state->rect) &&
    // input_is_pressed(resources->input, Button_Jump) &&
    // !map_scene->interpreter.event && *state->event_name)
    // {
    //     for (int i = 0; i < resources->event_loader->events->len; i++)
    //     {
    //         Event *event = linked_list_at(resources->event_loader->events,
    //         i); if (!strcmp(event->name, state->event_name))
    //         {
    //             interpreter_init(&map_scene->interpreter, event);
    //             return;
    //         }
    //     }
    //     log_warn("No such event `%s`", state->event_name);
    // }
}

void basic_char_free(void *self, Resources *resources, MapScene *map_scene)
{
    (void)map_scene;
    BasicCharState *state = self;
    if (state->texture)
    {
        sprite_free(&state->sprite, resources->graphics);
        layer_remove(&resources->graphics->sprite_layers.middle,
                     state->layer_entry);
    }
    free(state);
}

const CharacterInterface BASIC_CHARACTER_INTERFACE = {
    .name = "basic",
    .init_fn = basic_char_init,
    .update_fn = basic_char_update,
    .free_fn = basic_char_free};
