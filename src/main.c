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
#include "events/interpreter.h"
#include "player.h"

int main(void)
{
    SDL_Window *window;
    bool first_frame = true;

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

    window = SDL_CreateWindow("i am the window", 640, 480, SDL_WINDOW_HIDDEN);
    SDL_PTR_ERRCHK(window, "window creation failure");

    graphics_init(&graphics, window);

    char *files[] =
    {
        "assets/events.txt",
    };
    Interpreter *interpreter = interpreter_init(files, 1);

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

        if (first_frame)
        {
            SDL_ShowWindow(window);
            first_frame = false;
        }
    }

    graphics_free(&graphics);
    audio_free(&audio);
    SDL_Quit();

    return 0;
}
