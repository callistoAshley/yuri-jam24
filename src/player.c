#include "player.h"
#include "core_types.h"
#include "input/input.h"
#include "utility/common_defines.h"

void player_init(Player *player, Graphics *graphics, Physics *physics)
{
    player->camera = (Camera){0, 0, 0};

    player->transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry =
        transform_manager_add(&graphics->transform_manager, player->transform);

    // player is 8x16px (1x2 tiles/meters)
    Rect rect = rect_centered_from_size((vec2s){.x = 8, .y = 16});
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

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){1, 4};
    player->body_id = b2CreateBody(physics->world, &bodyDef);

    b2Polygon dynamicBox =
        b2MakeBox(rect.max.x / PX_PER_M, rect.max.y / PX_PER_M);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.0f;
    b2CreatePolygonShape(player->body_id, &shapeDef, &dynamicBox);

    // hacky ground box
    b2BodyDef groundBodyDef = b2DefaultBodyDef();
    groundBodyDef.position = (b2Vec2){25, -2.5};

    b2BodyId groundId = b2CreateBody(physics->world, &groundBodyDef);
    b2Polygon groundBox = b2MakeBox(25.0f, 2.5f);
    b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);
}

#define WALK_SPEED_MPS 6
#define WALK_SEED_PXPS M_TO_PX(WALK_SPEED_MPS)
void player_update(Player *player, Graphics *graphics, Input *input)
{
    if (input_is_down(input, Button_Left))
        b2Body_ApplyForceToCenter(player->body_id, (b2Vec2){-WALK_SPEED_MPS, 0},
                                  true);
    if (input_is_down(input, Button_Right))
        b2Body_ApplyForceToCenter(player->body_id, (b2Vec2){WALK_SPEED_MPS, 0},
                                  true);

    b2Vec2 body_position = b2Body_GetPosition(player->body_id);
    b2Rot rotation = b2Body_GetRotation(player->body_id);
    float angle = b2Rot_GetAngle(rotation);
    // box2d has a different coordinate system than us
    // +y is up for box2d, down for us
    // so we need to negate the y component
    player->transform.position.x = body_position.x * PX_PER_M;
    player->transform.position.y = -body_position.y * PX_PER_M;
    // we need to convert the rotation to a quaternion
    player->transform.rotation = glms_quatv(-angle, GLMS_ZUP);

    player->camera.x =
        player->transform.position.x - INTERNAL_SCREEN_WIDTH / 2.0;
    player->camera.y =
        player->transform.position.y - INTERNAL_SCREEN_HEIGHT / 2.0;

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
