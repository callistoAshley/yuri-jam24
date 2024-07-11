#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#define FATAL(...) return fprintf(stderr, __VA_ARGS__), EXIT_FAILURE;

int main(int argc, char *argv[])
{
    SDL_GLContext gl_context;
    SDL_Window *window;

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        FATAL("ERROR: SDL initialization error: %s\n", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetSwapInterval(1);

    window = SDL_CreateWindow(
        "i am the window",
        640,
        480,
        SDL_WINDOW_OPENGL
    );
    if (!window)
    {
        FATAL("ERROR: Window creation failure: %s\n", SDL_GetError());
    }
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        FATAL("ERROR: GL context creation failure: %s\n", SDL_GetError());
    }
    if (SDL_GL_MakeCurrent(window, gl_context))
    {
        FATAL("ERROR: SDL_GL_MakeCurrent failure: %s\n", SDL_GetError());
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        FATAL("ERROR: GLAD initialization failure.\n");
    }

    printf("GL Version: %s\n", glGetString(GL_VERSION));

    while (true)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) exit(EXIT_SUCCESS);
                    break;
                case SDL_EVENT_QUIT:
                    exit(EXIT_SUCCESS);
                    break;
                default: break;
            }
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
        SDL_Delay(1 / 60);
    }

    SDL_Quit();
}
