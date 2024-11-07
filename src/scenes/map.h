#pragma once

#include "graphics/tilemap.h"
#include "scene.h"
#include "player.h"
#include "ui/settings.h"
#include "ui/textbox.h"

// NOTE: MUST BE PASSED TO scene_change!!!!!!
typedef struct
{
    char *map_path;
    // NOTE: must be set to true if the path is a statically
    // allocated string
    bool copy_map_path;

    vec2s initial_position;
    bool has_initial_position;
} MapInitArgs;

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

    MapInitArgs change_map_args;
    bool change_map;
} MapScene;

extern const SceneInterface MAP_SCENE;
