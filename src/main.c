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
#include "input/input.h"
#include "utility/macros.h"

int main(void)
{
    SDL_Window *window;

    // Graphics stuff
    Graphics graphics;

    // audio things
    Audio audio;
    audio_init(&audio);

    Input input;
    input_new(&input);

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS),
               "SDL initialization failure");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    window = SDL_CreateWindow("i am the window", 640, 480, SDL_WINDOW_OPENGL);
    SDL_PTR_ERRCHK(window, "window creation failure");

    graphics_init(&graphics, window);

    while (!input_is_down(&input, Button_Quit))
    {
        SDL_Event event;

        input_start_frame(&input);
        while (SDL_PollEvent(&event))
        {
            input_process(&event, &input);
        }

        FMOD_Studio_System_Update(audio.system);

        graphics_render(&graphics);

        SDL_GL_SwapWindow(window);
        SDL_Delay(1 / 60);
    }

    audio_free(&audio);
    SDL_Quit();

    return 0;
}
