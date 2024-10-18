#include "map.h"
#include "events/interpreter.h"
#include "fmod_studio.h"
#include "input/input.h"
#include "player.h"
#include "ui/settings.h"
#include "ui/textbox.h"
#include "utility/common_defines.h"
#include "utility/macros.h"
#include "map_loader.h"
#include "characters/character.h"

#include <tmx.h>

typedef struct
{
    CharacterInterface interface;
    void *state;
} MapCharacterEntry; // FIXME: give this a better name

static char *tiled_image_path_to_actual(char *path)
{
    const char *prefix = "assets/textures/";
    char *new_path = malloc(strlen(prefix) + strlen(path) + 1);
    strcpy(new_path, prefix);
    strcat(new_path, path);
    return new_path;
}

void map_scene_init(Scene **scene_data, Resources *resources, void *extra_args)
{
    MapInitArgs *args = (MapInitArgs *)extra_args;

    MapScene *map_scene = malloc(sizeof(MapScene));
    map_scene->type = Scene_Map;
    *scene_data = (Scene *)map_scene;

    if (args->copy_map_path)
        map_scene->current_map = strdup(args->map_path);
    else
        map_scene->current_map = args->map_path;
    map_scene->should_free_current_map = args->copy_map_path;

    vec_init(&map_scene->colliders, sizeof(b2BodyId));
    vec_init(&map_scene->renderables, sizeof(MapRenderable));
    vec_init(&map_scene->characters, sizeof(MapCharacterEntry));

    FMOD_RESULT result;
    if (!resources->audio->current_bgm)
    {
        FMOD_STUDIO_EVENTDESCRIPTION *desc;
        result = FMOD_Studio_System_GetEvent(resources->audio->system,
                                             "event:/bgm/morning", &desc);
        FMOD_ERRCHK(result, "Failed to get event");
        FMOD_Studio_EventDescription_CreateInstance(
            desc, &resources->audio->current_bgm);
        FMOD_Studio_EventInstance_Start(resources->audio->current_bgm);
    }

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
        texture_manager_load(&resources->graphics->texture_manager, actual_path,
                             &resources->graphics->wgpu);
    free(actual_path);

    Transform transform = transform_from_xyz(0, 0, 0);
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, transform);

    tilemap_init(&map_scene->tilemap, resources->graphics, tileset_texture,
                 transform_entry, load.width, load.height, load.layers,
                 load.tiles);

    free(load.tiles);

    player_init(&map_scene->player, player_position, resources);

    settings_menu_init(&map_scene->settings, resources);
    textbox_init(&map_scene->textbox, resources);

    for (usize i = 0; i < load_characters.len; i++)
    {
        MapCharacterObj *obj = vec_get(&load_characters, i);
        void *state;
        obj->interface.init_fn(&state, resources, map_scene, obj->rect,
                               &obj->properties, NULL);

        MapCharacterEntry *entry = malloc(sizeof(MapCharacterEntry));
        entry->interface = obj->interface;
        entry->state = state;
        vec_push(&map_scene->characters, entry);

        hashmap_free(&obj->properties);
    }

    Event *event = linked_list_at(resources->event_loader->events, 0);
    interpreter_init(&map_scene->interpreter, event);

    vec_free(&load_characters);
    tmx_map_free(map);
}

void map_scene_update(Scene *scene_data, Resources *resources)
{
    MapScene *map_scene = (MapScene *)scene_data;

    interpreter_update(&map_scene->interpreter, resources);

    if (input_is_pressed(resources->input, Button_Back))
        map_scene->settings.open = true;

    if (input_is_pressed(resources->input, Button_Refresh))
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

    player_update(&map_scene->player, resources,
                  map_scene->freecam || map_scene->settings.open ||
                      map_scene->textbox.sprite.opacity > 0.0);

    f32 delta = duration_as_secs(resources->time.real->time.delta);
    if (map_scene->freecam)
    {
        bool left_down = input_is_down(resources->input, Button_Left);
        bool right_down = input_is_down(resources->input, Button_Right);
        bool up_down = input_is_down(resources->input, Button_Up);
        bool down_down = input_is_down(resources->input, Button_Down);

        const float freecam_move_speed = M_TO_PX(16) * delta;
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
        resources->raw_camera->x = map_scene->player.transform.position.x -
                                   INTERNAL_SCREEN_WIDTH / 2.0;
        resources->raw_camera->y = map_scene->player.transform.position.y -
                                   INTERNAL_SCREEN_HEIGHT / 2.0;

        // clamp the camera within the bounds of the map
        float right_edge =
            (map_scene->tilemap.map_w * 8) - INTERNAL_SCREEN_WIDTH;
        float bottom_edge =
            (map_scene->tilemap.map_h * 8) - INTERNAL_SCREEN_HEIGHT;
        if (resources->raw_camera->x < 0)
            resources->raw_camera->x = 0;
        else if (resources->raw_camera->x > right_edge)
            resources->raw_camera->x = right_edge;
        if (resources->raw_camera->y < 0)
            resources->raw_camera->y = 0;
        else if (resources->raw_camera->y > bottom_edge)
            resources->raw_camera->y = bottom_edge;
    }

    for (u32 i = 0; i < map_scene->characters.len; i++)
    {
        MapCharacterEntry *chara = vec_get(&map_scene->characters, i);
        chara->interface.update_fn(chara->state, resources, map_scene);
    }

    textbox_update(&map_scene->textbox, resources);
}

void map_scene_free(Scene *scene_data, Resources *resources)
{
    MapScene *map_scene = (MapScene *)scene_data;
    tilemap_free(&map_scene->tilemap, resources->graphics);
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
            sprite_free(renderable->data.sprite.ptr, resources->graphics);
            Layer *layer;
            switch (renderable->data.sprite.layer)
            {
            case Layer_Back:
                layer = &resources->graphics->sprite_layers.background;
                break;
            case Layer_Middle:
                layer = &resources->graphics->sprite_layers.middle;
                break;
            case Layer_Front:
                layer = &resources->graphics->sprite_layers.foreground;
                break;
            }
            layer_remove(layer, renderable->entry);
            break;
        }
        case Map_TileLayer:
        {
            free(renderable->data.tile.ptr);
            Layer *layer;
            switch (renderable->data.sprite.layer)
            {
            case Layer_Back:
                layer = &resources->graphics->tilemap_layers.background;
                break;
            case Layer_Middle:
                layer = &resources->graphics->tilemap_layers.middle;
                break;
            case Layer_Front:
                layer = &resources->graphics->tilemap_layers.foreground;
                break;
            }
            layer_remove(layer, renderable->entry);
            break;
        }
        case Map_Caster:
        {
            free(renderable->data.caster);
            layer_remove(&resources->graphics->shadowcasters,
                         renderable->entry);
            break;
        }
        case Map_Light:
            free(renderable->data.light);
            layer_remove(&resources->graphics->lights, renderable->entry);
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

    textbox_free(&map_scene->textbox, resources);

    // hack to clear shadow casters, under the assumption that they'll all get
    // loaded again
    caster_manager_clear(&resources->graphics->caster_manager);

    free(map_scene);
}

const SceneInterface MAP_SCENE = {
    .init = map_scene_init,
    .update = map_scene_update,
    .free = map_scene_free,
};
