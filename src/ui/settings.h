#pragma once

#include "graphics/layer.h"
#include "graphics/ui_sprite.h"
#include "scenes/scene.h"

typedef enum
{
    Cat_Audio,
    Cat_Video,
    Cat_Controls,
    Cat_Back,
    Cat_None,
} SettingsCategory;

typedef struct
{
    UiSprite background;
    LayerEntry bg_entry;

    UiSprite categories[4];
    LayerEntry cat_entries[4];

    SettingsCategory hovered_category;
    SettingsCategory selected_category;

    FMOD_STUDIO_EVENTDESCRIPTION *hover_desc;
    FMOD_STUDIO_EVENTDESCRIPTION *click_desc;

    bool open;
    bool is_closing;
} SettingsMenu;

void settings_menu_init(SettingsMenu *menu, Resources *resources);

void settings_menu_update(SettingsMenu *menu, Resources *resources);

void settings_menu_free(SettingsMenu *menu, Resources *resources);
