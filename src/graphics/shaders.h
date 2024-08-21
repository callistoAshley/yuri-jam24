#pragma once

#include <wgpu.h>
#include "utility/list.h"
#include "webgpu.h"
#include "wgpu_resources.h"
#include "bind_group_layouts.h"
#include <stdalign.h>
#include <cglm/struct.h>

typedef struct Shaders
{
    WGPURenderPipeline object;
    WGPURenderPipeline lighting;
    WGPURenderPipeline tilemap;

    struct
    {
        WGPURenderPipeline line;
        // used for boxes and polygons
        WGPURenderPipeline polygon;
        // used for circles and points
        WGPURenderPipeline circle;
        WGPURenderPipeline capsule;
    } box2d_debug;
} Shaders;

typedef struct
{
    vec3s color;
    // you'd think we should put this alignas on the color field, but it doesn't
    // work for some reason?
    alignas(16) vec2s camera_position;
    vec2s position;
    f32 radius;
    f32 internal_scale;
    u32 solid;
} B2DCirclePushConstants;

typedef struct
{
    mat4s camera;
    u32 transform_index;
    u32 texture_index;
} ObjectPushConstants;

typedef struct
{
    mat4s camera;
    u32 transform_index;
    u32 texture_index;
    u32 map_width;
} TilemapPushConstants;

void shaders_init(Shaders *shaders, BindGroupLayouts *layouts,
                  WGPUResources *resources);
