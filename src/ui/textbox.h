#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "fmod_studio_common.h"
#include "graphics/layer.h"
#include "graphics/ui_sprite.h"
#include "scenes/scene.h"

typedef struct
{
    UiSprite sprite;
    LayerEntry sprite_entry;

    UiSprite text_sprite;
    LayerEntry text_sprite_entry;

    FMOD_STUDIO_EVENTDESCRIPTION *talk_sound;

    char text[512];
    int text_idx;
    f32 text_type_time;

    bool open;
    bool typing, waiting_for_input;
    bool needs_remove_text, fixed_update_occured;
} Textbox;

void textbox_init(Textbox *textbox, Resources *resources);
void textbox_free(Textbox *textbox, Resources *resources);
void textbox_fixed_update(Textbox *textbox, Resources *resources);
void textbox_update(Textbox *textbox, Resources *resources);
void textbox_display_text(Textbox *textbox, Resources *resources, char *text);
