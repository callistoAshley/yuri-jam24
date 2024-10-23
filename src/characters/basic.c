#include "basic.h"
#include "scenes/map.h"
#include "utility/log.h"
#include "input/input.h"
#include <string.h>

void *basic_char_init(Resources *resources, struct MapScene *map_scene,
                      CharacterInitArgs *args)
{
    (void)map_scene;

    BasicCharState *state = malloc(sizeof(BasicCharState));
    state->rect = args->rect;
    memset(state->sprite_name, 0, sizeof(state->sprite_name));
    memset(state->event_name, 0, sizeof(state->event_name));

    if (hashmap_get(args->metadata, "sprite"))
    {
        state->transform =
            transform_from_xyz(args->rect.min.x, args->rect.min.y, 0);
        TransformEntry transform = transform_manager_add(
            &resources->graphics->transform_manager, state->transform);

        strncpy(state->sprite_name, hashmap_get(args->metadata, "sprite"),
                sizeof(state->sprite_name));
        TextureEntry *texture = texture_manager_load(
            &resources->graphics->texture_manager, state->sprite_name,
            &resources->graphics->wgpu);
        WGPUTexture wgpu_tex = texture_manager_get_texture(
            &resources->graphics->texture_manager, texture);
        u32 width = wgpuTextureGetWidth(wgpu_tex),
            height = wgpuTextureGetHeight(wgpu_tex);

        state->quad =
            quad_init(rect_from_size((vec2s){.x = width, .y = height}),
                      RECT_UNIT_TEX_COORDS);

        sprite_init(
            &state->sprite, texture, transform,
            quad_manager_add(&resources->graphics->quad_manager, state->quad));
        state->layer_entry = layer_add(
            &resources->graphics->sprite_layers.middle, &state->sprite);
    }

    if (hashmap_get(args->metadata, "event"))
    {
        strncpy(state->event_name, hashmap_get(args->metadata, "event"),
                sizeof(state->event_name));
    }

    return state;
}

void basic_char_update(void *self, Resources *resources, MapScene *map_scene)
{
    BasicCharState *state = self;

    Rect player_rect =
        rect_from_min_size((vec2s){.x = map_scene->player.transform.position.x,
                                   .y = map_scene->player.transform.position.y},
                           (vec2s){.x = PLAYER_W, .y = PLAYER_H});

    // FIXME: this comparison does not work if there are multiple vms running
    if (rect_contains_other(player_rect, state->rect) &&
        input_is_pressed(resources->input, Button_Interact) &&
        !map_scene->vms.len && *state->event_name)
    {
        for (u32 i = 0; i < resources->event_count; i++)
        {
            Event event = resources->events[i];
            if (!strcmp(event.name, state->event_name))
            {
                VM vm;
                vm_init(&vm, event);
                vec_push(&map_scene->vms, &vm);
                return;
            }
        }
        log_warn("No such event `%s`", state->event_name);
    }
}

void basic_char_free(void *self, Resources *resources, MapScene *map_scene)
{
    (void)map_scene;
    BasicCharState *state = self;
    if (state->sprite.texture)
    {
        sprite_free(&state->sprite, resources->graphics);
        layer_remove(&resources->graphics->sprite_layers.middle,
                     state->layer_entry);
    }
    free(state);
}
