#pragma once

#include "graphics/sprite.h"
#include "scene.h"

typedef struct
{
    SceneType type;

    Sprite background;
    Sprite options[3];
    i32 selected_option;
} TitleScene;

extern const SceneInterface TITLE_SCENE;
