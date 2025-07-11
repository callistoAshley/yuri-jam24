#include "basic.h"
#include "animation/animation.h"
#include "animation/definition.h"
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

    if (hashmap_get(args->metadata, "animation"))
    {
        const char *animation_name = hashmap_get(args->metadata, "animation");
        AnimationType type = anim_type_for(animation_name);
        animation_init(&state->animation, type);
    }

    if (hashmap_get(args->metadata, "sprite"))
    {
        state->transform =
            transform_from_xyz(args->rect.min.x, args->rect.min.y, 0);
        TransformEntry transform = transform_manager_add(
            &resources->graphics.transform_manager, state->transform);

        char sprite_name[256] = {0};
        strncpy(sprite_name, hashmap_get(args->metadata, "sprite"),
                sizeof(sprite_name) - 1);
        TextureEntry *texture =
            texture_manager_load(&resources->graphics.texture_manager,
                                 sprite_name, &resources->graphics.wgpu);
        WGPUTexture wgpu_tex = texture_manager_get_texture(
            &resources->graphics.texture_manager, texture);
        u32 width = wgpuTextureGetWidth(wgpu_tex),
            height = wgpuTextureGetHeight(wgpu_tex);

        if (state->animation.def)
        {
            width = state->animation.def->cell_width;
            height = state->animation.def->cell_height;
        }

        state->quad =
            quad_init(rect_from_size((vec2s){.x = width, .y = height}),
                      RECT_UNIT_TEX_COORDS);

        sprite_init(
            &state->sprite, texture, transform,
            quad_manager_add(&resources->graphics.quad_manager, state->quad));
        state->layer_entry = layer_add(
            &resources->graphics.sprite_layers.middle, &state->sprite);
    }

    if (hashmap_get(args->metadata, "event"))
    {
        strncpy(state->event_name, hashmap_get(args->metadata, "event"),
                sizeof(state->event_name) - 1);
    }

    if (state->animation.def)
    {
        animation_apply(&state->animation, &resources->graphics, &state->quad,
                        &state->sprite);
    }

    return state;
}

void basic_char_update(void **self, Resources *resources, MapScene *map_scene)
{
    BasicCharState *state = *self;

    if (state->animation.def)
    {
        animation_update(&state->animation, resources);
        animation_apply(&state->animation, &resources->graphics, &state->quad,
                        &state->sprite);
    }

    Rect player_rect =
        rect_from_min_size((vec2s){.x = map_scene->player.transform.position.x,
                                   .y = map_scene->player.transform.position.y},
                           (vec2s){.x = PLAYER_W, .y = PLAYER_H});

    bool player_inside = rect_contains_other(player_rect, state->rect);
    bool interact_pressed = input_did_press(&resources->input, Button_Interact);
    bool has_event = (*state->event_name) != '\0';

    if (player_inside && interact_pressed && !state->vm && has_event)
    {
        for (u32 i = 0; i < resources->event_count; i++)
        {
            Event event = resources->events[i];
            if (!strcmp(event.name, state->event_name))
            {
                state->vm = malloc(sizeof(VM));
                vm_init(state->vm, event);
                state->vm->vm_ctx = state;
                return;
            }
        }
        log_warn("No such event `%s`", state->event_name);
    }
}

void basic_char_fixed_update(void **self, Resources *resources,
                             MapScene *map_scene)
{
    (void)map_scene;

    BasicCharState *state = *self;
    if (state->vm)
    {
        bool finished = vm_execute(state->vm, resources);
        if (finished)
        {
            vm_free(state->vm);
            free(state->vm);
            state->vm = NULL;
        }
    }
}

void basic_char_free(void *self, Resources *resources, MapScene *map_scene)
{
    (void)map_scene;
    BasicCharState *state = self;
    if (state->sprite.texture)
    {
        sprite_free(&state->sprite, &resources->graphics);
        layer_remove(&resources->graphics.sprite_layers.middle,
                     state->layer_entry);
    }

    if (state->vm)
    {
        vm_free(state->vm);
        free(state->vm);
    }

    if (state->animation.def)
    {
        animation_free(&state->animation);
    }

    free(state);
}
