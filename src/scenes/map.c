#include "map.h"
#include "input/input.h"
#include "player.h"
#include "resources.h"
#include "ui/settings.h"
#include "ui/textbox.h"
#include "utility/common_defines.h"
#include "utility/macros.h"
#include "map_loader.h"
#include "characters/character.h"

#include <tmx.h>
#include <stddef.h>

typedef struct
{
    CharacterInterface interface;
    void *state;
} MapCharacterEntry; // FIXME: give this a better name

static Layer *layer_for(StandardLayers *layers, MapLayer map_layer)
{
    switch (map_layer)
    {
    case Layer_Back:
        return &layers->background;
        break;
    case Layer_Middle:
        return &layers->middle;
        break;
    case Layer_Front:
        return &layers->foreground;
        break;
    }
    unreachable();
}

static char *tiled_image_path_to_actual(char *path)
{
    const char *prefix = "assets/textures/";
    char *new_path = malloc(strlen(prefix) + strlen(path) + 1);
    strcpy(new_path, prefix);
    strcat(new_path, path);
    return new_path;
}

void map_scene_init(Resources *resources, void *extra_args)
{
    MapInitArgs *args = (MapInitArgs *)extra_args;

    MapScene *map_scene = malloc(sizeof(MapScene));
    resources->scene = (Scene *)map_scene;
    map_scene->type = Scene_Map;

    if (args->copy_map_path)
        map_scene->current_map = strdup(args->map_path);
    else
        map_scene->current_map = args->map_path;
    map_scene->should_free_current_map = args->copy_map_path;
    map_scene->change_map = false;

    vec_init(&map_scene->colliders, sizeof(b2BodyId));
    vec_init(&map_scene->renderables, sizeof(MapRenderable));
    vec_init(&map_scene->characters, sizeof(MapCharacterEntry));

    map_scene->freecam = false;

    tmx_map *map = tmx_load(args->map_path);
    if (!map)
    {
        FATAL("Failed to load map %s: %s", args->map_path, tmx_strerr());
    }

    // check that we've only got one tileset!!!
    INV_PTR_ERRCHK(map->ts_head->next, "Map only has one tileset");
    // check that it's a tile atlas as well, rather than a collection of images
    PTR_ERRCHK(map->ts_head->tileset->image,
               "Tileset does not have an image source");

    vec load_characters;
    vec_init(&load_characters, sizeof(MapCharacterObj));

    b2Vec2 player_position = {0, 0};
    MapLoadArgs load = {
        .tiles = NULL,
        .width = map->width,
        .height = map->height,
        .layers = 0,
        .player_position = &player_position,
        .colliders = &map_scene->colliders,
        .renderables = &map_scene->renderables,
        .characters = &load_characters,
        .tilemap = &map_scene->tilemap,
    };
    handle_map_layers(map->ly_head, resources, &load);

    char *actual_path =
        tiled_image_path_to_actual(map->ts_head->tileset->image->source);
    TextureEntry *tileset_texture =
        texture_manager_load(&resources->graphics.texture_manager, actual_path,
                             &resources->graphics.wgpu);
    free(actual_path);

    Transform transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics.transform_manager, transform);

    tilemap_init(&map_scene->tilemap, &resources->graphics, tileset_texture,
                 transform_entry, load.width, load.height, load.layers,
                 load.tiles);

    free(load.tiles);

    player_init(&map_scene->player, player_position, resources);

    settings_menu_init(&map_scene->settings, resources);
    inventory_init(&map_scene->inventory, resources);
    textbox_init(&map_scene->textbox, resources);

    for (usize i = 0; i < load_characters.len; i++)
    {
        MapCharacterObj *obj = vec_get(&load_characters, i);
        CharacterInitArgs args = {
            .rect = obj->rect,
            .rotation = obj->rotation,
            .object_type = obj->object_type,
            .metadata = &obj->properties,
            .extra_args = NULL,
        };
        void *state = obj->interface.init_fn(resources, map_scene, &args);

        MapCharacterEntry *entry = malloc(sizeof(MapCharacterEntry));
        entry->interface = obj->interface;
        entry->state = state;
        vec_push(&map_scene->characters, entry);

        hashmap_free(&obj->properties);
    }

    vec_free(&load_characters);
    tmx_map_free(map);
}

void map_scene_fixed_update(Resources *resources)
{
    MapScene *map_scene = (MapScene *)resources->scene;

    for (u32 i = 0; i < map_scene->characters.len; i++)
    {
        MapCharacterEntry *chara = vec_get(&map_scene->characters, i);
        if (chara->interface.fixed_update_fn)
        {
            chara->interface.fixed_update_fn(&chara->state, resources,
                                             map_scene);
        }
    }

    // if an event signaled to us that we need to change maps, change maps
    if (map_scene->change_map)
    {
        // copy args out because the scene will be free'd
        MapInitArgs args = map_scene->change_map_args;
        scene_change(MAP_SCENE, resources, &args);
        return; // return IMMEDIATELY. the current scene is no longer valid!!!
    }

    textbox_fixed_update(&map_scene->textbox, resources);

    player_fixed_update(&map_scene->player, resources);
}

void map_scene_update(Resources *resources)
{
    MapScene *map_scene = (MapScene *)resources->scene;

    bool menu_is_open = map_scene->settings.open || map_scene->textbox.open ||
                        map_scene->inventory.open;
    if (input_did_press(&resources->input, Button_Back) && !menu_is_open)
    {
        map_scene->settings.open = true;
        menu_is_open = true;
    }
    if (input_did_press(&resources->input, Button_Inventory) && !menu_is_open)
    {
        map_scene->inventory.open = true;
        map_scene->inventory.opening = true;
        menu_is_open = true;
    }

    if (input_did_press(&resources->input, Button_Refresh))
    {
        MapInitArgs args = {.map_path = map_scene->current_map,
                            .copy_map_path = false};
        // don't free the current map path because the reloaded scene will do it
        // for us
        map_scene->should_free_current_map = false;
        scene_change(MAP_SCENE, resources, &args);
        return; // return IMMEDIATELY. the current scene is no longer valid!!!
    }

    settings_menu_update(&map_scene->settings, resources);
    inventory_update(&map_scene->inventory, resources);

    f32 delta = duration_as_secs(resources->time.current.delta);
    if (map_scene->freecam)
    {
        bool left_down = input_is_down(&resources->input, Button_Left);
        bool right_down = input_is_down(&resources->input, Button_Right);
        bool up_down = input_is_down(&resources->input, Button_Up);
        bool down_down = input_is_down(&resources->input, Button_Down);

        const float freecam_move_speed = M_TO_PX(16) * delta;
        if (right_down)
            resources->raw_camera.x += freecam_move_speed;
        if (left_down)
            resources->raw_camera.x -= freecam_move_speed;
        if (up_down)
            resources->raw_camera.y -= freecam_move_speed;
        if (down_down)
            resources->raw_camera.y += freecam_move_speed;
    }
    else
    {
        resources->raw_camera.x =
            map_scene->player.transform.position.x - GAME_VIEW_WIDTH / 2.0;
        resources->raw_camera.y =
            map_scene->player.transform.position.y - GAME_VIEW_HEIGHT / 2.0;

        // clamp the camera within the bounds of the map
        float right_edge = (map_scene->tilemap.map_w * 8) - GAME_VIEW_WIDTH;
        float bottom_edge = (map_scene->tilemap.map_h * 8) - GAME_VIEW_HEIGHT;
        if (resources->raw_camera.x < 0)
            resources->raw_camera.x = 0;
        else if (resources->raw_camera.x > right_edge)
            resources->raw_camera.x = right_edge;
        if (resources->raw_camera.y < 0)
            resources->raw_camera.y = 0;
        else if (resources->raw_camera.y > bottom_edge)
            resources->raw_camera.y = bottom_edge;
    }

    for (u32 i = 0; i < map_scene->characters.len; i++)
    {
        MapCharacterEntry *chara = vec_get(&map_scene->characters, i);
        if (chara->interface.update_fn)
        {
            chara->interface.update_fn(&chara->state, resources, map_scene);
        }
    }

    textbox_update(&map_scene->textbox, resources);

    bool disable_input = map_scene->freecam || menu_is_open;
    player_update(&map_scene->player, resources, disable_input);
}

void map_scene_free(Resources *resources)
{
    MapScene *map_scene = (MapScene *)resources->scene;
    tilemap_free(&map_scene->tilemap, &resources->graphics);
    player_free(&map_scene->player, resources);

    if (map_scene->should_free_current_map)
        free(map_scene->current_map);

    for (u32 i = 0; i < map_scene->colliders.len; i++)
    {
        b2BodyId *body_id = vec_get(&map_scene->colliders, i);
        b2DestroyBody(*body_id);
    }
    vec_free(&map_scene->colliders);

    for (u32 i = 0; i < map_scene->renderables.len; i++)
    {
        MapRenderable *renderable = vec_get(&map_scene->renderables, i);
        switch (renderable->type)
        {
        case Map_Sprite:
        {
            sprite_free(renderable->data.sprite.ptr, &resources->graphics);
            Layer *layer = layer_for(&resources->graphics.sprite_layers,
                                     renderable->data.sprite.layer);
            layer_remove(layer, renderable->entry);
            break;
        }
        case Map_TileLayer:
        {
            free(renderable->data.tile.ptr);
            Layer *layer = layer_for(&resources->graphics.tilemap_layers,
                                     renderable->data.tile.layer);
            layer_remove(layer, renderable->entry);
            break;
        }
        case Map_Light:
            free(renderable->data.light);
            layer_remove(&resources->graphics.lights, renderable->entry);
            break;
        }
    }
    vec_free(&map_scene->renderables);

    for (u32 i = 0; i < map_scene->characters.len; i++)
    {
        MapCharacterEntry *chara = vec_get(&map_scene->characters, i);
        chara->interface.free_fn(chara->state, resources, map_scene);
    }
    vec_free(&map_scene->characters);

    settings_menu_free(&map_scene->settings, resources);
    inventory_free(&map_scene->inventory, resources);
    textbox_free(&map_scene->textbox, resources);

    free(map_scene);
}

const SceneInterface MAP_SCENE = {
    .init = map_scene_init,
    .update = map_scene_update,
    .fixed_update = map_scene_fixed_update,
    .free = map_scene_free,
};
