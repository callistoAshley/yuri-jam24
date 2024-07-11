#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

typedef struct
{
    SDL_GLContext ctx;
} Graphics;

void graphics_init(Graphics *graphics, SDL_Window *window);
void graphics_render(Graphics *graphics);
// freeing the graphics context is done by SDL itself