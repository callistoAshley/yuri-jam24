#pragma once

#include "wgpu_resources.h"
#include "quad_manager.h"
#include "shaders.h"
#include "player.h"

typedef struct Graphics
{
    WGPUResources wgpu;
    Shaders shaders;
    QuadManager quad_manager;
} Graphics;

void graphics_init(Graphics *graphics, SDL_Window *window);
void graphics_render(Graphics *graphics);
void graphics_free(Graphics *graphics);
