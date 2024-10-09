#pragma once

#include "fmod_studio_common.h"
#include "graphics/layer.h"
#include "graphics/ui_sprite.h"
#include "scene.h"
#include "ui/settings.h"

typedef struct
{
    SceneType type;

    SettingsMenu settings_menu;

    UiSprite background;
    LayerEntry background_entry;

    UiSprite options[3];
    LayerEntry option_entries[3];

    i32 hovered_option;

    FMOD_STUDIO_EVENTDESCRIPTION *hover_desc;
    FMOD_STUDIO_EVENTDESCRIPTION *click_desc;

    bool is_transitioning;
    f32 transition_timer;
} TitleScene;

extern const SceneInterface TITLE_SCENE;
