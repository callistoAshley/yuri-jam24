#include <fmod_errors.h>
#include <fmod_studio.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio/audio.h"
#include "utility/macros.h"

int main(int argc, char *argv[])
{
    SDL_GLContext gl_context;
    SDL_Window *window;

    // audio things
    Audio audio;
    audio_init(&audio);

    FMOD_STUDIO_EVENTDESCRIPTION *what_once_was;
    FMOD_STUDIO_EVENTINSTANCE *what_once_was_instance;
    int event_progression = 0;
    FMOD_RESULT result = 0;

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS),
               "SDL initialization failure");

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

    SDL_ERRCHK(SDL_GL_MakeCurrent(window, gl_context),
               "SDL_GL_MakeCurrent failure");

    printf("GL Version: %s\n", glGetString(GL_VERSION));

    result = FMOD_Studio_System_GetEvent(
        audio.system, "event:/bgm_what_once_was", &what_once_was);
    FMOD_ERRCHK(result, "Getting event description");

    result = FMOD_Studio_EventDescription_CreateInstance(
        what_once_was, &what_once_was_instance);
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
                    FMOD_Studio_EventInstance_SetParameterByName(
                        what_once_was_instance, "Progression",
                        (float)event_progression, false);
                }
                if (event.key.key == SDLK_RIGHT)
                {
                    if (event_progression < 5)
                        event_progression++;
                    FMOD_Studio_EventInstance_SetParameterByName(
                        what_once_was_instance, "Progression",
                        (float)event_progression, false);
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
        glClearColor(0.2 * (float)event_progression, 0.0, 0.0,
                     1.0); // progression ranges from 0 to 5
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
        SDL_Delay(1 / 60);
    }

    audio_free(&audio);
    SDL_Quit();
}
