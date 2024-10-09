#include "audio.h"
#include "sensible_nums.h"
#include "utility/macros.h"

void audio_init(Audio *audio, bool with_liveupdate)
{
    unsigned int fmod_version;
    FMOD_RESULT result = 0;
    // initialize FMOD
    result = FMOD_Studio_System_Create(&audio->system, FMOD_VERSION);
    FMOD_ERRCHK(result, "Creating system");
    u32 flags = FMOD_STUDIO_INIT_NORMAL;
    if (with_liveupdate)
        flags |= FMOD_STUDIO_INIT_LIVEUPDATE;
    result = FMOD_Studio_System_Initialize(audio->system, 48, flags,
                                           FMOD_INIT_NORMAL, NULL);
    FMOD_ERRCHK(result, "Initializing system");

    // fetch the core system
    FMOD_Studio_System_GetCoreSystem(audio->system, &audio->core_system);

    // print the FMOD version
    FMOD_System_GetVersion(audio->core_system, &fmod_version, NULL);
    printf("FMOD Version: %d.%d.%d\n", fmod_version >> 16,
           fmod_version >> 8 & 0xFF, fmod_version & 0xFF);

    result = FMOD_Studio_System_LoadBankFile(
        audio->system, "assets/audio/Desktop/Master.bank",
        FMOD_STUDIO_LOAD_BANK_NORMAL, &audio->master_bank);
    FMOD_ERRCHK(result, "Loading Master Bank");
    result = FMOD_Studio_System_LoadBankFile(
        audio->system, "assets/audio/Desktop/Master.strings.bank",
        FMOD_STUDIO_LOAD_BANK_NORMAL, &audio->strings_bank);
    FMOD_ERRCHK(result, "Loading Master Strings Bank");
    result = FMOD_Studio_System_LoadBankFile(
        audio->system, "assets/audio/Desktop/BGM.bank",
        FMOD_STUDIO_LOAD_BANK_NORMAL, &audio->bgm_bank);
    FMOD_ERRCHK(result, "Loading BGM Bank");
    result = FMOD_Studio_System_LoadBankFile(
        audio->system, "assets/audio/Desktop/SFX.bank",
        FMOD_STUDIO_LOAD_BANK_NORMAL, &audio->sfx_bank);
    FMOD_ERRCHK(result, "Loading Menu Bank");

    audio->current_bgm = NULL;
}

void audio_free(Audio *audio)
{
    FMOD_RESULT result = 0;

    result = FMOD_Studio_System_Release(audio->system);
    FMOD_ERRCHK(result, "Releasing system");
}
