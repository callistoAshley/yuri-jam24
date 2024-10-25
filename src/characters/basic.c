#include "basic.h"
#include "scenes/map.h"
#include "utility/log.h"
#include "input/input.h"
#include <string.h>

void *basic_char_init(Resources *resources, struct MapScene *map_scene,
                      CharacterInitArgs *args)
{
    (void)map_scene;

    BasicCharState *state = calloc(1, sizeof(BasicCharState));
    state->rect = args->rect;

    if (hashmap_get(args->metadata, "sprite"))
    {
        state->transform =
            transform_from_xyz(args->rect.min.x, args->rect.min.y, 0);
        TransformEntry transform = transform_manager_add(
            &resources->graphics->transform_manager, state->transform);

        char sprite_name[256] = {0};
        strncpy(sprite_name, hashmap_get(args->metadata, "sprite"),
                sizeof(sprite_name));
        TextureEntry *texture =
            texture_manager_load(&resources->graphics->texture_manager,
                                 sprite_name, &resources->graphics->wgpu);
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

    if (hashmap_get(args->metadata, "shadow"))
    {
        char caster_name[256] = {0};
        strncpy(caster_name, hashmap_get(args->metadata, "shadow"),
                sizeof(caster_name));

        f32 radius = -1;

        char *radius_text = hashmap_get(args->metadata, "shadow_radius");
        if (radius_text)
        {
            radius = atof(radius_text);
        }

        CasterEntry *caster_entry = caster_manager_load(
            &resources->graphics->caster_manager, caster_name);
        state->caster = (ShadowCaster){
            .caster = caster_entry,
            .cell = 0,
            .radius = radius,
            .transform = state->sprite.transform,
        };
        state->caster_entry =
            layer_add(&resources->graphics->shadowcasters, &state->caster);
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

    if (state->caster.caster)
    {
        layer_remove(&resources->graphics->shadowcasters, state->caster_entry);
    }

    free(state);
}
