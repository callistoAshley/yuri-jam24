#pragma once

#include "debug/level_editor.h"
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

    bool freecam, level_editor_enabled;
    LevelEditor *editor;
} MapScene;

extern const SceneInterface MAP_SCENE;
