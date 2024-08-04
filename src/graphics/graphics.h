#pragma once

#include "graphics/tex_manager.h"
#include "input/input.h"
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
    TextureManager texture_manager;
    WGPUSampler sampler;

    WGPUTexture color;
    WGPUTextureView color_view;
    WGPUTexture normal;
    WGPUTextureView normal_view;
} Graphics;

void graphics_init(Graphics *graphics, SDL_Window *window);
void graphics_render(Graphics *graphics, Input *input);
void graphics_free(Graphics *graphics);
