#pragma once

#include "graphics/layer.h"
#include "graphics/ui_sprite.h"
#include "scenes/scene.h"

typedef struct
{
    UiSprite background;
    LayerEntry bg_entry;

    bool open;
} SettingsMenu;

void settings_menu_init(SettingsMenu *menu, Resources *resources);

void settings_menu_update(SettingsMenu *menu, Resources *resources);

void settings_menu_free(SettingsMenu *menu, Resources *resources);
