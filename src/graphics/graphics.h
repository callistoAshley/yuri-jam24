#pragma once

#include <wgpu.h>
#include <SDL3/SDL.h>

#include "shaders.h"
#include "player.h"

typedef struct Graphics
{
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUSurfaceConfiguration surface_config;
    Shaders shaders;
} Graphics;

void graphics_init(Graphics *graphics, SDL_Window *window);
void graphics_render(Graphics *graphics, Player *player);
void graphics_free(Graphics *graphics);
