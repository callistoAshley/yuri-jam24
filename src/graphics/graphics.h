#pragma once

#include "wgpu_resources.h"
#include "shaders.h"
#include "player.h"

typedef struct Graphics
{
    WGPUResources wgpu;
    Shaders shaders;
} Graphics;

void graphics_init(Graphics *graphics, SDL_Window *window);
void graphics_render(Graphics *graphics, Player *player);
void graphics_free(Graphics *graphics);
