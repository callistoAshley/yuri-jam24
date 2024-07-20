#pragma once

#include "transform_manager.h"
#include "wgpu_resources.h"
#include "quad_manager.h"
#include "shaders.h"
#include "bind_group_layouts.h"
#include "player.h"

typedef struct Graphics
{
    WGPUResources wgpu;

    Shaders shaders;
    BindGroupLayouts bind_group_layouts;

    QuadManager quad_manager;
    TransformManager transform_manager;

    WGPUBindGroup transform_bind_group;
} Graphics;

void graphics_init(Graphics *graphics, SDL_Window *window);
void graphics_render(Graphics *graphics);
void graphics_free(Graphics *graphics);
