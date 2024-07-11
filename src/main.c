#include <fmod_errors.h>
#include <fmod_studio.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "graphics/graphics.h"
#include "audio/audio.h"
#include "utility/macros.h"

int main(int argc, char *argv[])
{
    SDL_Window *window;

    // Graphics stuff
    Graphics graphics;

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

    graphics_init(&graphics, window);

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

        graphics_render(&graphics);

        SDL_GL_SwapWindow(window);
        SDL_Delay(1 / 60);
    }

    audio_free(&audio);
    SDL_Quit();
}
