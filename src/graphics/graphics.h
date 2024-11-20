#pragma once

#include "graphics/layer.h"
#include "graphics/tex_manager.h"
#include "physics/physics.h"
#include "settings.h"
#include "transform_manager.h"
#include "wgpu_resources.h"
#include "quad_manager.h"
#include "shaders.h"
#include "bind_group_layouts.h"

typedef struct
{
    Layer background;
    Layer middle;
    Layer foreground;
} StandardLayers;

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

    // after rendering to the color texture, we do deffered shading
    // and render the result to the lit texture. this texture is then upscaled,
    // tone mapped, and drawn to the screen.
    WGPUTexture lit;
    WGPUTextureView lit_view;

    StandardLayers sprite_layers;

    // why is this separate from the sprite layers? well, layers are set up to
    // draw only ONE type of thing. why? because we want to minimize the amount
    // of pipeline + bind group changes we have to do.
    StandardLayers tilemap_layers;

    StandardLayers ui_layers;

    Layer lights;
} Graphics;

typedef struct
{
    f32 x, y, z;
} Camera;

void graphics_init(Graphics *graphics, SDL_Window *window, Settings *settings);
void graphics_render(Graphics *graphics, Physics *physics, Camera camera);
void graphics_free(Graphics *graphics);
void graphics_resize(Graphics *graphics, int width, int height);
QuadEntry graphics_screen_quad_entry(void);

// TODO add convenience functions that forward to the appropriate manager
