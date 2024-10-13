#pragma once

#include "settings.h"
#include <fmod_errors.h>
#include <fmod_studio.h>
#include <stdbool.h>

typedef struct
{
    FMOD_STUDIO_SYSTEM *system;
    FMOD_SYSTEM *core_system;

    FMOD_STUDIO_BANK *master_bank;
    FMOD_STUDIO_BANK *strings_bank;
    FMOD_STUDIO_BANK *bgm_bank;
    FMOD_STUDIO_BANK *sfx_bank;

    FMOD_STUDIO_EVENTINSTANCE *current_bgm;

    FMOD_STUDIO_BUS *bgm_bus;
    FMOD_STUDIO_BUS *sfx_bus;
} Audio;

void audio_init(Audio *audio, bool with_liveupdate, Settings *settings);
void audio_free(Audio *audio);
