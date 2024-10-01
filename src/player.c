#include "player.h"
#include "box2d/box2d.h"
#include "cglm/struct/vec2.h"
#include "core_types.h"
#include "graphics/caster_manager.h"
#include "scenes/scene.h"
#include "utility/common_defines.h"
#include "utility/log.h"

void player_init(Player *player, b2Vec2 initial_pos, Resources *resources)
{
    player->transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, player->transform);

    // player is 8x16px (1x2 tiles/meters)
    Rect rect = rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 8, .y = 16});
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

    CasterEntry *caster =
        caster_manager_load(&resources->graphics->caster_manager,
                            "assets/shadowcasters/player.shdw");

    player->shadow_caster.caster = caster;
    player->shadow_caster.cell = 0;
    player->shadow_caster.transform = transform_entry;

    player->shadow_caster_entry =
        layer_add(&resources->graphics->shadowcasters, &player->shadow_caster);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = initial_pos;
    bodyDef.fixedRotation = true;
    player->body_id = b2CreateBody(resources->physics->world, &bodyDef);

    b2Polygon dynamicBox = b2MakeBox(4.0 / PX_PER_M, 8.0 / PX_PER_M);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.0f;
    player->shape_id =
        b2CreatePolygonShape(player->body_id, &shapeDef, &dynamicBox);

    player->jump_timeout = 0;
}

#define WALK_SPEED_MPS 4
#define WALK_SEED_PXPS M_TO_PX(WALK_SPEED_MPS)
void player_update(Player *player, Resources *resources, bool disable_input)
{
    bool left_down =
        input_is_down(resources->input, Button_Left) && !disable_input;
    bool right_down =
        input_is_down(resources->input, Button_Right) && !disable_input;

    f32 walk_speed = WALK_SEED_PXPS * resources->input->delta_seconds;
    if (left_down)
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){-walk_speed, 0}, true);
    if (right_down)
        b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                          (b2Vec2){walk_speed, 0}, true);

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
        if (input_is_down(resources->input, Button_Up) && contact_is_ground &&
            !disable_input)
        {
            b2Body_ApplyLinearImpulseToCenter(player->body_id,
                                              (b2Vec2){0.0, 15.0}, true);
            player->jump_timeout = 0.1;
        }
    }

    b2Vec2 body_position = b2Body_GetPosition(player->body_id);
    // box2d has a different coordinate system than us
    // +y is up for box2d, down for us
    // so we need to negate the y component
    player->transform.position.x = body_position.x * PX_PER_M - 4;
    player->transform.position.y = -body_position.y * PX_PER_M - 8;
    // we need to convert the rotation to a quaternion

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
