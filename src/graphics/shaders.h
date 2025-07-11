#pragma once

#include <wgpu.h>
#include "cglm/types-struct.h"
#include "webgpu.h"
#include "wgpu_resources.h"
#include "bind_group_layouts.h"
#include <stdalign.h>
#include <cglm/struct.h>

typedef struct Shaders
{
    struct
    {
        WGPURenderPipeline ui_sprite;
        WGPURenderPipeline hdr_tonemap;
    } forward;

    struct
    {
        WGPURenderPipeline sprite;
        WGPURenderPipeline tilemap;
    } defferred;

    struct
    {
        WGPURenderPipeline point;
        WGPURenderPipeline direct;
    } lights;

    struct
    {
        // used for boxes and polygons
        WGPURenderPipeline polygon;
        // used for circles and points
        WGPURenderPipeline circle;
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
    vec3s color;
    // you'd think we should put this alignas on the color field, but it doesn't
    // work for some reason?
    alignas(16) vec2s camera_position;
    vec2s position;
    vec2s rotation;
    f32 internal_scale;
    u32 solid;
} B2DDrawPolygonPushConstants;

typedef struct
{
    mat4s camera;
    u32 transform_index;
    u32 texture_index;
} SpritePushConstants;

typedef struct
{
    mat4s camera;
    u32 transform_index;
    u32 texture_index;
    f32 opacity;
} UiSpritePushConstants;

typedef struct
{
    mat4s camera;
    u32 transform_index;
    u32 texture_index;
    u32 map_width;
} TilemapPushConstants;

typedef struct
{
    vec3s color;

    alignas(16) vec2s position;
    vec2s camera_position;

    f32 intensity;
    f32 radius;
    f32 volumetric_intensity;

    alignas(16) vec3s mask_tex_offset;
} PointLightPushConstants;

typedef struct
{
    vec3s color;
    f32 intensity;
    f32 volumetric_intensity;

    alignas(16) vec3s mask_tex_offset;
} DirectLightPushConstants;

void shaders_init(Shaders *shaders, BindGroupLayouts *layouts,
                  WGPUResources *resources);
void shaders_free(Shaders *shaders);
