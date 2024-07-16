#include <SDL3/SDL_video.h>
#include <fmod_errors.h>
#include <fmod_studio.h>

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "graphics/graphics.h"
#include "audio/audio.h"
#include "input/input.h"
#include "utility/macros.h"
#include "player.h"

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

    Player player = PLAYER_INIT;

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS),
               "SDL initialization failure");

    window = SDL_CreateWindow("i am the window", 640, 480, 0);
    SDL_PTR_ERRCHK(window, "window creation failure");

    graphics_init(&graphics, window);
    graphics_render(&graphics,
                    &player); // render once to initialize the swapchain

    while (!input_is_down(&input, Button_Quit))
    {
        SDL_Event event;

        input_start_frame(&input);
        while (SDL_PollEvent(&event))
        {
            input_process(&event, &input);
        }

        FMOD_Studio_System_Update(audio.system);

        graphics_render(&graphics, &player);
        SDL_Delay(16); // this doesn't handle vsync properly
    }

    graphics_free(&graphics);
    audio_free(&audio);
    SDL_Quit();

    return 0;
}
