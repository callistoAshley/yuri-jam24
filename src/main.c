#include <fmod_errors.h>
#include <fmod_studio.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define FATAL(...) fprintf(stderr, __VA_ARGS__), exit(1);
#define SDL_ERRCHK(expr, msg)                                           \
    {                                                                   \
        if (expr)                                                       \
        {                                                               \
            FATAL("SDL error:" msg ": %s\n", SDL_GetError());          \
        }                                                               \
    }
#define SDL_PTR_ERRCHK(expr, msg) SDL_ERRCHK(!expr, msg)
#define FMOD_ERRCHK(expr, msg)                                          \
{                                                                       \
    if (expr != FMOD_OK)                                              \
    {                                                                   \
        FATAL("FMOD error: " msg ": %s\n", FMOD_ErrorString(result)); \
    }                                                                   \
}

typedef struct {
  FMOD_STUDIO_SYSTEM *system;
  FMOD_SYSTEM *core_system;

  FMOD_STUDIO_BANK *master_bank;
  FMOD_STUDIO_BANK *strings_bank;
  FMOD_STUDIO_BANK *bgm_bank;
} Audio;

Audio audio_init() {
  Audio audio = {0};
  unsigned int fmod_version;
  FMOD_RESULT result = 0;
  // initialize FMOD
  result = FMOD_Studio_System_Create(&audio.system, FMOD_VERSION);
  FMOD_ERRCHK(result, "Creating system");
  result = FMOD_Studio_System_Initialize(
      audio.system, 48, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, NULL);
  FMOD_ERRCHK(result, "Initializing system");

  // fetch the core system
  FMOD_Studio_System_GetCoreSystem(audio.system, &audio.core_system);

  // print the FMOD version
  FMOD_System_GetVersion(audio.core_system, &fmod_version, NULL);
  printf("FMOD Version: %d.%d.%d\n", fmod_version >> 16, fmod_version >> 8 & 0xFF, 
      fmod_version & 0xFF);

  result = FMOD_Studio_System_LoadBankFile(audio.system, "assets/audio/Master.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &audio.master_bank);
  FMOD_ERRCHK(result, "Loading Master Bank");
  result = FMOD_Studio_System_LoadBankFile(audio.system, "assets/audio/Master.strings.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &audio.strings_bank);
  FMOD_ERRCHK(result, "Loading Master Strings Bank");
  result = FMOD_Studio_System_LoadBankFile(audio.system, "assets/audio/BGM.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &audio.bgm_bank);
  FMOD_ERRCHK(result, "Loading Master BGM Bank");

  return audio;
}

void audio_free(Audio audio) {
  FMOD_RESULT result = 0;

  result = FMOD_Studio_System_Release(audio.system);
  FMOD_ERRCHK(result, "Releasing system");
}

int main(int argc, char *argv[]) {
    SDL_GLContext gl_context;
    SDL_Window *window;
    // audio things
    Audio audio = audio_init();
    FMOD_STUDIO_EVENTDESCRIPTION *what_once_was;
    FMOD_STUDIO_EVENTINSTANCE *what_once_was_instance;
    int event_progression = 0;
    FMOD_RESULT result = 0;

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS), "SDL initialization failure");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    window = SDL_CreateWindow("i am the window", 640, 480, SDL_WINDOW_OPENGL);
    SDL_PTR_ERRCHK(window, "window creation failure");

    gl_context = SDL_GL_CreateContext(window);
    SDL_PTR_ERRCHK(gl_context, "GL context creation failure")

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) 
    {
        FATAL("ERROR: GLAD initialization failure.\n");
    }

    SDL_ERRCHK(SDL_GL_MakeCurrent(window, gl_context), "SDL_GL_MakeCurrent failure");

    printf("GL Version: %s\n", glGetString(GL_VERSION));

    result = FMOD_Studio_System_GetEvent(audio.system, "event:/bgm_what_once_was", &what_once_was);
    FMOD_ERRCHK(result, "Getting event description");

    result = FMOD_Studio_EventDescription_CreateInstance(what_once_was, &what_once_was_instance);
    FMOD_ERRCHK(result, "Creating event instance");
    result = FMOD_Studio_EventInstance_Start(what_once_was_instance);
    FMOD_ERRCHK(result, "Starting event instance");

    while (true) 
    {
        SDL_Event event;

        while (SDL_PollEvent(&event)) 
        {
            switch (event.type) 
            {
              case SDL_EVENT_KEY_DOWN:
                  if (event.key.key == SDLK_ESCAPE)
                      exit(EXIT_SUCCESS);
                  if (event.key.key == SDLK_LEFT)
                  {
                      if (event_progression > 0)
                          event_progression--;
                      FMOD_Studio_EventInstance_SetParameterByName(what_once_was_instance, "Progression", (float) event_progression, false);
                  }
                  if (event.key.key == SDLK_RIGHT)
                  {
                      if (event_progression < 5)
                          event_progression++;
                      FMOD_Studio_EventInstance_SetParameterByName(what_once_was_instance, "Progression", (float) event_progression, false);
                  }
                  break;
              case SDL_EVENT_QUIT:
                  exit(EXIT_SUCCESS);
                  break;
              default:
                  break;
            }
        }

        FMOD_Studio_System_Update(audio.system);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClearColor(0.2 * (float) event_progression, 0.0, 0.0, 1.0); // progression ranges from 0 to 5
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
        SDL_Delay(1 / 60);
    }

    audio_free(audio);
    SDL_Quit();
}
