#pragma once

#include "fmod_studio_common.h"
#include "graphics/ui_sprite.h"
#include "scene.h"

typedef struct
{
    SceneType type;

    UiSprite background;
    UiSprite options[3];

    FMOD_STUDIO_EVENTINSTANCE *bgm;
    i32 selected_option;
} TitleScene;

extern const SceneInterface TITLE_SCENE;
