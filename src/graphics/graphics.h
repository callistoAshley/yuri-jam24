#pragma once

#include "graphics/layer.h"
#include "graphics/tex_manager.h"
#include "input/input.h"
#include "transform_manager.h"
#include "wgpu_resources.h"
#include "quad_manager.h"
#include "shaders.h"
#include "bind_group_layouts.h"
#include "player.h"

#define INTERNAL_SCREEN_WIDTH 160
#define INTERNAL_SCREEN_HEIGHT 90

#define WINDOW_SCALE 8
#define WINDOW_WIDTH (INTERNAL_SCREEN_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (INTERNAL_SCREEN_HEIGHT * WINDOW_SCALE)

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

    struct
    {
        Layer background;
        Layer middle;
        Layer foreground;
    } sprite_layers;

    // why is this separate from the sprite layers? well, layers are set up to
    // draw only ONE type of thing. why? because we want to minimize the amount
    // of pipeline + bind group changes we have to do.
    struct
    {
        Layer background;
        Layer middle;
        Layer foreground;
    } tilemap_layers;
} Graphics;

void graphics_init(Graphics *graphics, SDL_Window *window);
void graphics_render(Graphics *graphics, Input *input);
void graphics_free(Graphics *graphics);
void graphics_resize(Graphics *graphics, int width, int height);