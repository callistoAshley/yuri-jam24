#include "player.h"
#include "cglm/struct/vec2.h"
#include "core_types.h"

void player_init(Player *player, Graphics *graphics, Physics *physics)
{
    player->camera = (Camera){0, 0, 0};

    player->transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry =
        transform_manager_add(&graphics->transform_manager, player->transform);

    // player is 8x16px (1x2 tiles/meters)
    Rect rect = rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 8, .y = 16});
    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
    player->quad = (Quad){rect, tex_coords};
    QuadEntry quad_entry =
        quad_manager_add(&graphics->quad_manager, player->quad);

    TextureEntry *texture =
        texture_manager_load(&graphics->texture_manager,
                             "assets/textures/player.png", &graphics->wgpu);

    sprite_init(&player->sprite, texture, transform_entry, quad_entry);

    player->layer_entry =
        layer_add(&graphics->sprite_layers.middle, &player->sprite);
}

#define WALK_SPEED_MPS 3
#define WALK_SEED_PXPS M_TO_PX(WALK_SPEED_MPS)
void player_update(Player *player, Graphics *graphics, Input *input)
{

    if (input_is_down(input, Button_Down))
        player->transform.position.y += WALK_SEED_PXPS * input->delta_seconds;

    if (input_is_down(input, Button_Up))
        player->transform.position.y -= WALK_SEED_PXPS * input->delta_seconds;

    if (input_is_down(input, Button_Left))
        player->transform.position.x -= WALK_SEED_PXPS * input->delta_seconds;

    if (input_is_down(input, Button_Right))
        player->transform.position.x += WALK_SEED_PXPS * input->delta_seconds;

    player->camera.x =
        player->transform.position.x + 4 - INTERNAL_SCREEN_WIDTH / 2.0;
    player->camera.y =
        player->transform.position.y + 8 - INTERNAL_SCREEN_HEIGHT / 2.0;

    transform_manager_update(&graphics->transform_manager,
                             player->sprite.transform, player->transform);
}

void player_free(Player *player, Graphics *graphics)
{
    // we only need to remove the sprite from the layer-
    // sprite_free will handle the rest
    layer_remove(&graphics->sprite_layers.middle, player->layer_entry);
    sprite_free(&player->sprite, graphics);
}
