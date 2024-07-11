#include <glad/glad.h>

#include "graphics.h"
#include "utility/macros.h"

void graphics_init(Graphics *graphics, SDL_Window *window)
{
    graphics->ctx = SDL_GL_CreateContext(window);
    SDL_PTR_ERRCHK(graphics->ctx, "GL context creation failure")

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        FATAL("ERROR: GLAD initialization failure.\n");
    }

    SDL_ERRCHK(SDL_GL_MakeCurrent(window, graphics->ctx),
               "SDL_GL_MakeCurrent failure");
}

void graphics_render(Graphics *graphics)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}