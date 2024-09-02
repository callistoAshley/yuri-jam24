#include "player.h"
#include "core_types.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"
#include "utility/log.h"

void player_init(Player *player, Resources *resources)
{
    player->transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, player->transform);

    // player is 8x16px (1x2 tiles/meters)
    Rect rect = rect_centered_from_size((vec2s){.x = 8, .y = 16});
    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
    player->quad = (Quad){rect, tex_coords};
    QuadEntry quad_entry =
        quad_manager_add(&resources->graphics->quad_manager, player->quad);

    TextureEntry *texture = texture_manager_load(
        &resources->graphics->texture_manager, "assets/textures/player.png",
        &resources->graphics->wgpu);

    sprite_init(&player->sprite, texture, transform_entry, quad_entry);

    player->layer_entry =
        layer_add(&resources->graphics->sprite_layers.middle, &player->sprite);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){1, 4};
    bodyDef.fixedRotation = true;
    player->body_id = b2CreateBody(resources->physics->world, &bodyDef);

    b2Polygon dynamicBox =
        b2MakeBox(rect.max.x / PX_PER_M, rect.max.y / PX_PER_M);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.0f;
    player->shape_id =
        b2CreatePolygonShape(player->body_id, &shapeDef, &dynamicBox);

    // hacky ground box
    b2BodyDef groundBodyDef = b2DefaultBodyDef();
    groundBodyDef.position = (b2Vec2){25, -2.5};

    b2BodyId groundId = b2CreateBody(resources->physics->world, &groundBodyDef);
    b2Polygon groundBox = b2MakeBox(25.0f, 2.5f);
    b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    player->jump_timeout = 0;
}

#define WALK_SPEED_MPS 6
#define WALK_SEED_PXPS M_TO_PX(WALK_SPEED_MPS)
void player_update(Player *player, Resources *resources)
{
#ifdef DEBUG
    bool freecam_down = input_is_down(resources->input, Button_Freecam);
    bool up_down = input_is_down(resources->input, Button_Up);
    bool down_down = input_is_down(resources->input, Button_Down);
#endif
    bool left_down = input_is_down(resources->input, Button_Left);
    bool right_down = input_is_down(resources->input, Button_Right);

#ifdef DEBUG
    if (!freecam_down)
    {
#endif
        if (left_down)
            b2Body_ApplyForceToCenter(player->body_id,
                                      (b2Vec2){-WALK_SPEED_MPS, 0}, true);
        if (right_down)
            b2Body_ApplyForceToCenter(player->body_id,
                                      (b2Vec2){WALK_SPEED_MPS, 0}, true);
        // if we're not moving, we want to stop quickly
        if (left_down || right_down)
            b2Shape_SetFriction(player->shape_id, 0.0f);
        else
            b2Shape_SetFriction(player->shape_id, 0.5f);

        // check if we're on the ground
        b2ContactData contact_data[8];
        int contact_count =
            b2Shape_GetContactData(player->shape_id, contact_data, 8);
        if (player->jump_timeout > 0)
            player->jump_timeout -= resources->input->delta_seconds;

        if (contact_count > 0 && player->jump_timeout <= 0)
        {
            bool contact_is_ground = false;
            for (int i = 0; i < contact_count; i++)
            {
                // figure out what direction the normal is in (player->ground or
                // ground->player)
                b2ContactData contact = contact_data[i];
                bool player_is_a =
                    contact.shapeIdA.index1 == player->shape_id.index1;
                b2Vec2 normal = player_is_a ? contact.manifold.normal
                                            : b2Neg(contact.manifold.normal);
                // normal is facing down, which means we're on the ground
                if (normal.y < 0.0)
                {
                    contact_is_ground = true;
                    break;
                }
            }
            // we're on the ground
            if (input_is_down(resources->input, Button_Up) && contact_is_ground)
            {
                b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                                  (b2Vec2){0.0, 15.0}, true);
                player->jump_timeout = 0.1;
            }
        }
#ifdef DEBUG
    }
#endif

    b2Vec2 body_position = b2Body_GetPosition(player->body_id);
    // box2d has a different coordinate system than us
    // +y is up for box2d, down for us
    // so we need to negate the y component
    player->transform.position.x = body_position.x * PX_PER_M;
    player->transform.position.y = -body_position.y * PX_PER_M;
    // we need to convert the rotation to a quaternion

#ifdef DEBUG
    if (freecam_down)
    {
        const float freecam_move_speed =
            M_TO_PX(16) * resources->input->delta_seconds;
        if (right_down)
            resources->raw_camera->x += freecam_move_speed;
        if (left_down)
            resources->raw_camera->x -= freecam_move_speed;
        if (up_down)
            resources->raw_camera->y -= freecam_move_speed;
        if (down_down)
            resources->raw_camera->y += freecam_move_speed;
    }
    else
    {
#endif
        resources->raw_camera->x =
            player->transform.position.x - INTERNAL_SCREEN_WIDTH / 2.0;
        resources->raw_camera->y =
            player->transform.position.y - INTERNAL_SCREEN_HEIGHT / 2.0;
#ifdef DEBUG
    }
#endif

    transform_manager_update(&resources->graphics->transform_manager,
                             player->sprite.transform, player->transform);
}

void player_free(Player *player, Resources *resources)
{
    // we only need to remove the sprite from the layer-
    // sprite_free will handle the rest
    layer_remove(&resources->graphics->sprite_layers.middle,
                 player->layer_entry);
    sprite_free(&player->sprite, resources->graphics);
}
