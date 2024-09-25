#pragma once

#include "graphics/tilemap.h"
#include "scene.h"
#include "player.h"

typedef struct
{
    // because this struct starts with SceneType (and so does Scene) we can cast
    // between them
    SceneType type;

    Tilemap tilemap;
    Player player;

    bool freecam;
} MapScene;

// NOTE: MUST BE PASSED TO scene_change!!!!!!
typedef struct
{
    char *map_path;
} MapInitArgs;

extern const SceneInterface MAP_SCENE;
