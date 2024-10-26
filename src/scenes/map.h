#pragma once

#include "events/vm.h"
#include "graphics/tilemap.h"
#include "scene.h"
#include "player.h"
#include "ui/settings.h"
#include "ui/textbox.h"

typedef struct MapScene
{
    // because this struct starts with SceneType (and so does Scene) we can cast
    // between them
    SceneType type;

    Tilemap tilemap;
    Player player;

    bool freecam;

    vec colliders;
    vec renderables;
    vec characters;

    char *current_map;
    bool should_free_current_map;

    SettingsMenu settings;
    Textbox textbox;
} MapScene;

// NOTE: MUST BE PASSED TO scene_change!!!!!!
typedef struct
{
    char *map_path;
    bool copy_map_path;
} MapInitArgs;

void map_scene_init(Scene **scene_data, Resources *resources, void *extra_args);
void map_scene_update(Scene *scene_data, Resources *resources);
void map_scene_free(Scene *scene_data, Resources *resources);

extern const SceneInterface MAP_SCENE;
