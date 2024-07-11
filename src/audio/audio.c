#include "audio.h"
#include "utility/macros.h"

#include <stdio.h>

void audio_init(Audio *audio)
{
    unsigned int fmod_version;
    FMOD_RESULT result = 0;
    // initialize FMOD
    result = FMOD_Studio_System_Create(&audio->system, FMOD_VERSION);
    FMOD_ERRCHK(result, "Creating system");
    result = FMOD_Studio_System_Initialize(
        audio->system, 48, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, NULL);
    FMOD_ERRCHK(result, "Initializing system");

    // fetch the core system
    FMOD_Studio_System_GetCoreSystem(audio->system, &audio->core_system);

    // print the FMOD version
    FMOD_System_GetVersion(audio->core_system, &fmod_version, NULL);
    printf("FMOD Version: %d.%d.%d\n", fmod_version >> 16,
           fmod_version >> 8 & 0xFF, fmod_version & 0xFF);

    result = FMOD_Studio_System_LoadBankFile(
        audio->system, "assets/audio/Master.bank", FMOD_STUDIO_LOAD_BANK_NORMAL,
        &audio->master_bank);
    FMOD_ERRCHK(result, "Loading Master Bank");
    result = FMOD_Studio_System_LoadBankFile(
        audio->system, "assets/audio/Master.strings.bank",
        FMOD_STUDIO_LOAD_BANK_NORMAL, &audio->strings_bank);
    FMOD_ERRCHK(result, "Loading Master Strings Bank");
    result = FMOD_Studio_System_LoadBankFile(
        audio->system, "assets/audio/BGM.bank", FMOD_STUDIO_LOAD_BANK_NORMAL,
        &audio->bgm_bank);
    FMOD_ERRCHK(result, "Loading Master BGM Bank");
}

void audio_free(Audio *audio)
{
    FMOD_RESULT result = 0;

    result = FMOD_Studio_System_Release(audio->system);
    FMOD_ERRCHK(result, "Releasing system");
}