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

void map_scene_init(Scene **scene_data, Resources *resources, void *extra_args);
void map_scene_update(Scene *scene_data, Resources *resources);
void map_scene_free(Scene *scene_data, Resources *resources);

extern const SceneInterface MAP_SCENE;
