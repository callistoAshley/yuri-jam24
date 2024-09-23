#pragma once

#include "fmod_studio_common.h"
#include "graphics/layer.h"
#include "graphics/ui_sprite.h"
#include "scene.h"

typedef struct
{
    SceneType type;

    UiSprite background;
    LayerEntry background_entry;

    UiSprite options[3];
    LayerEntry option_entries[3];

    FMOD_STUDIO_EVENTINSTANCE *bgm;
    i32 selected_option;
} TitleScene;

extern const SceneInterface TITLE_SCENE;
