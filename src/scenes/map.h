#pragma once

#include "graphics/tilemap.h"
#include "scene.h"
#include "player.h"

typedef struct
{
    Tilemap tilemap;
    Player player;
} MapScene;

extern const SceneInterface MAP_SCENE;
