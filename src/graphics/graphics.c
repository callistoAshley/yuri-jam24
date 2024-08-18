#include <stdio.h>
#include <wgpu.h>

#include "graphics.h"
#include "cglm/types-struct.h"
#include "core_types.h"
#include "binding_helper.h"
#include "graphics/layer.h"
#include "graphics/quad_manager.h"
#include "graphics/sprite.h"
#include "graphics/tex_manager.h"
#include "graphics/tilemap.h"
#include "imgui-wgpu.h"
#include "input/input.h"
#include "utility/log.h"
#include "utility/macros.h"
#include "webgpu.h"

int screen_quad_index;

Tilemap tilemap;

Sprite player_sprite;
int player_tex_width, player_tex_height;
Transform player_transform;

typedef struct
{
    Tilemap *tilemap;
    int layer;
} TilemapLayer;

void tilemap_layer_draw(void *layer, Graphics *graphics, mat4s camera,
                        WGPURenderPassEncoder pass)
{
    (void)graphics;
    TilemapLayer *tilemap_layer = layer;
    tilemap_render(tilemap_layer->tilemap, camera, tilemap_layer->layer, pass);
}

void sprite_draw(void *thing, Graphics *graphics, mat4s camera,
                 WGPURenderPassEncoder pass)
{
    (void)graphics;
    Sprite *sprite = thing;
    sprite_render(sprite, camera, pass);
}

void graphics_init(Graphics *graphics, SDL_Window *window)
{
    wgpu_resources_init(&graphics->wgpu, window);
    bind_group_layouts_init(&graphics->bind_group_layouts, &graphics->wgpu);
    shaders_init(&graphics->shaders, &graphics->bind_group_layouts,
                 &graphics->wgpu);

    quad_manager_init(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_init(&graphics->transform_manager, &graphics->wgpu);
    // texture manager has no gpu side resources allocated initially so no need
    // to pass wgpu
    texture_manager_init(&graphics->texture_manager);

    layer_init(&graphics->tilemap_layers.background, tilemap_layer_draw, free);
    layer_init(&graphics->tilemap_layers.middle, tilemap_layer_draw, free);
    layer_init(&graphics->tilemap_layers.foreground, tilemap_layer_draw, free);

    layer_init(&graphics->sprite_layers.background, sprite_draw, NULL);
    layer_init(&graphics->sprite_layers.middle, sprite_draw, NULL);
    layer_init(&graphics->sprite_layers.foreground, sprite_draw, NULL);

    // load bearing molly
    // why do we need this? primarily to make sure that at least one texture is
    // loaded, because you can't bind an empty texture array
    texture_manager_load(&graphics->texture_manager,
                         "assets/textures/load_bearing_molly.png",
                         &graphics->wgpu);

    WGPUExtent3D extents = {
        .width = INTERNAL_SCREEN_WIDTH,
        .height = INTERNAL_SCREEN_HEIGHT,
        .depthOrArrayLayers = 1,
    };
    WGPUTextureDescriptor desc = {
        .label = "color texture",
        .size = extents,
        .dimension = WGPUTextureDimension_2D,
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .usage =
            WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
    };
    graphics->color = wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
    graphics->color_view = wgpuTextureCreateView(graphics->color, NULL);

    desc.label = "normal texture";
    desc.format = WGPUTextureFormat_RGBA32Float;
    graphics->normal = wgpuDeviceCreateTexture(graphics->wgpu.device, &desc);
    graphics->normal_view = wgpuTextureCreateView(graphics->normal, NULL);

    graphics->sampler = wgpuDeviceCreateSampler(graphics->wgpu.device, NULL);

    // screen quad
    {
        Rect rect = {
            .min = {.x = -1.0, .y = 1.0},
            .max = {.x = 1.0, .y = -1.0},
        };
        Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
        Quad quad = {
            .rect = rect,
            .tex_coords = tex_coords,
        };
        screen_quad_index = quad_manager_add(&graphics->quad_manager, quad);
    }

    {
        Transform transform = transform_from_xyz(0, 0, 0);
        TransformEntry tilemap_transform =
            transform_manager_add(&graphics->transform_manager, transform);
        TextureEntry *tileset = texture_manager_load(
            &graphics->texture_manager, "assets/textures/red_start.png",
            &graphics->wgpu);
        u32 map_data[8 * 15];
        for (int i = 0; i < 8 * 15; i++)
        {
            map_data[i] = 1;
        }
        tilemap_new(&tilemap, graphics, tileset, tilemap_transform, 8, 15, 1,
                    map_data);

        TilemapLayer *background = malloc(sizeof(TilemapLayer));
        background->tilemap = &tilemap;
        background->layer = 0;
        layer_add(&graphics->tilemap_layers.background, background);
    }

    {
        player_transform = transform_from_xyz(0, 0, 0);
        TransformEntry transform_entry = transform_manager_add(
            &graphics->transform_manager, player_transform);

        TextureEntry *texture_entry =
            texture_manager_load(&graphics->texture_manager,
                                 "assets/textures/kitty.png", &graphics->wgpu);
        WGPUTexture texture = texture_manager_get_texture(
            &graphics->texture_manager, texture_entry);
        player_tex_width = wgpuTextureGetWidth(texture);
        player_tex_height = wgpuTextureGetHeight(texture);

        Rect rect =
            rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 32, .y = 32});
        Rect tex_coords =
            rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 32.0, .y = 32.0});
        Quad quad = {
            .rect = rect,
            .tex_coords = tex_coords,
        };
        quad = quad_norm_tex_coords(quad, player_tex_width, player_tex_height);
        QuadEntry player_quad = quad_manager_add(&graphics->quad_manager, quad);

        sprite_init(&player_sprite, texture_entry, transform_entry,
                    player_quad);
        layer_add(&graphics->sprite_layers.middle, &player_sprite);
    }
}

int camera_x = 0;
int camera_y = 0;

void build_object_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);
    bind_group_builder_append_texture_view_array(
        &builder,
        (WGPUTextureView *)graphics->texture_manager.texture_views.data,
        graphics->texture_manager.texture_views.len);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.object,
                                   "Transform Bind Group");

    bind_group_builder_free(&builder);
}

void build_light_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_init(&builder);

    bind_group_builder_append_texture_view(&builder, graphics->color_view);
    bind_group_builder_append_texture_view(&builder, graphics->normal_view);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.lighting,
                                   "Light Bind Group");

    bind_group_builder_free(&builder);
}

void build_tilemap_bind_group(Graphics *graphics, WGPUBindGroup *bind_group)
{
    // FIXME it's a waste to create a new bind group every frame + this has the
    // same layout as the regular object bind group
    BindGroupBuilder builder;
    bind_group_builder_init(&builder);

    bind_group_builder_append_buffer(&builder,
                                     graphics->transform_manager.buffer);
    bind_group_builder_append_texture_view_array(
        &builder,
        (WGPUTextureView *)graphics->texture_manager.texture_views.data,
        graphics->texture_manager.texture_views.len);
    bind_group_builder_append_sampler(&builder, graphics->sampler);

    *bind_group = bind_group_build(&builder, graphics->wgpu.device,
                                   graphics->bind_group_layouts.tilemap,
                                   "Tilemap Bind Group");

    bind_group_builder_free(&builder);
}

typedef enum
{
    idle,
    alert,
    scratchself,
    scratchwallN,
    scratchwallS,
    scratchwallE,
    scratchwallW,
    tired,
    sleeping,
    N,
    NE,
    E,
    SE,
    S,
    SW,
    W,
    NW,
} sprite_set;
typedef enum
{
    Dir_None = 0,
    Dir_N = 1 << 1,
    Dir_W = 1 << 2,
    Dir_S = 1 << 3,
    Dir_E = 1 << 4,
} Direction;
const int sprite_sets[][2][2] = {
    {
        {-3, -3},
        {-3, -3},
    },
    {
        {-7, -3},
        {-7, -3},
    },
    {
        {-5, 0},
        {-6, 0},
    },
    {
        {0, 0},
        {0, -1},
    },
    {
        {-7, -1},
        {-6, -2},
    },
    {
        {-2, -2},
        {-2, -3},
    },
    {
        {-4, 0},
        {-4, -1},
    },
    {
        {-3, -2},
        {-3, -2},
    },
    {
        {-2, 0},
        {-2, -1},
    },
    {
        {-1, -2},
        {-1, -3},
    },
    {
        {0, -2},
        {0, -3},
    },
    {
        {-3, 0},
        {-3, -1},
    },
    {
        {-5, -1},
        {-5, -2},
    },
    {
        {-6, -3},
        {-7, -2},
    },
    {
        {-5, -3},
        {-6, -1},
    },
    {
        {-4, -2},
        {-4, -3},
    },
    {
        {-1, 0},
        {-1, -1},
    },
};

void graphics_render(Graphics *graphics, Input *input)
{
    Direction direction = Dir_None;
    if (input_is_down(input, Button_Down))
    {
        direction |= Dir_S;
        player_transform.position.y++;
    }
    if (input_is_down(input, Button_Up))
    {
        direction |= Dir_N;
        player_transform.position.y--;
    }
    if (input_is_down(input, Button_Left))
    {
        direction |= Dir_W;
        player_transform.position.x--;
    }
    if (input_is_down(input, Button_Right))
    {
        direction |= Dir_E;
        player_transform.position.x++;
    }

    if (direction & Dir_S && direction & Dir_N)
    {
        direction ^= Dir_S | Dir_N;
    }
    if (direction & Dir_W && direction & Dir_E)
    {
        direction ^= Dir_W | Dir_E;
    }

    sprite_set set = idle;
    if (direction & Dir_N && direction & Dir_W)
        set = NW;
    else if (direction & Dir_N && direction & Dir_E)
        set = NE;
    else if (direction & Dir_S && direction & Dir_W)
        set = SW;
    else if (direction & Dir_S && direction & Dir_E)
        set = SE;
    else if (direction & Dir_N)
        set = N;
    else if (direction & Dir_W)
        set = W;
    else if (direction & Dir_S)
        set = S;
    else if (direction & Dir_E)
        set = E;

    int animation_frame = SDL_GetTicks() / 100 % 2;

    const int(*sprite_set)[2][2] = &sprite_sets[set];
    const int(*sprite)[2] = &(*sprite_set)[animation_frame];

    int tex_x = -(*sprite)[0];
    int tex_y = -(*sprite)[1];

    Rect rect = rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 32, .y = 32});
    Rect tex_coords =
        rect_from_min_size((vec2s){.x = tex_x * 32, .y = tex_y * 32},
                           (vec2s){.x = 32.0, .y = 32.0});
    Quad quad = {
        .rect = rect,
        .tex_coords = tex_coords,
    };
    quad = quad_norm_tex_coords(quad, player_tex_width, player_tex_height);

    camera_x = player_transform.position.x + 16 - INTERNAL_SCREEN_WIDTH / 2.0;
    camera_y = player_transform.position.y + 16 - INTERNAL_SCREEN_HEIGHT / 2.0;

    quad_manager_update(&graphics->quad_manager, player_sprite.quad, quad);

    transform_manager_update(&graphics->transform_manager,
                             player_sprite.transform, player_transform);

    quad_manager_upload_dirty(&graphics->quad_manager, &graphics->wgpu);
    transform_manager_upload_dirty(&graphics->transform_manager,
                                   &graphics->wgpu);

    WGPUBindGroup object_bind_group;
    build_object_bind_group(graphics, &object_bind_group);

    WGPUBindGroup light_bind_group;
    build_light_bind_group(graphics, &light_bind_group);

    WGPUBindGroup tilemap_bind_group;
    build_tilemap_bind_group(graphics, &tilemap_bind_group);

    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(graphics->wgpu.surface, &surface_texture);

    switch (surface_texture.status)
    {
    case WGPUSurfaceGetCurrentTextureStatus_Success:
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
    {
        // Skip this frame, and re-configure surface.
        if (surface_texture.texture != NULL)
        {
            wgpuTextureRelease(surface_texture.texture);
        }
        wgpuSurfaceConfigure(graphics->wgpu.surface,
                             &graphics->wgpu.surface_config);

        return;
    }
    case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
    case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
    case WGPUSurfaceGetCurrentTextureStatus_Force32:
        FATAL("WGPU surface texture status: %#.8x\n", surface_texture.status);
    }

    WGPUTextureView frame =
        wgpuTextureCreateView(surface_texture.texture, NULL);

    WGPUCommandEncoder command_encoder =
        wgpuDeviceCreateCommandEncoder(graphics->wgpu.device, NULL);

    WGPURenderPassColorAttachment object_attachments[] = {
        {
            .view = graphics->color_view,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f},
        },
        {
            .view = graphics->normal_view,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f},
        }};
    WGPURenderPassDescriptor object_render_pass_desc = {
        .label = "object render pass encoder",
        .colorAttachmentCount = 2,
        .colorAttachments = object_attachments,
    };
    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
        command_encoder, &object_render_pass_desc);

    mat4s camera_projection = glms_ortho(
        0.0, INTERNAL_SCREEN_WIDTH, INTERNAL_SCREEN_HEIGHT, 0.0, -1.0f, 1.0f);
    mat4s camera_transform =
        glms_look((vec3s){.x = camera_x, .y = camera_y, .z = 0},
                  (vec3s){.x = 0.0, .y = 0.0, .z = -1.0},
                  (vec3s){.x = 0.0, .y = 1.0, .z = 0.0});
    mat4s camera = glms_mat4_mul(camera_projection, camera_transform);
    u64 quad_buffer_size = wgpuBufferGetSize(graphics->quad_manager.buffer);

    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.tilemap);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                      NULL);
    layer_draw(&graphics->tilemap_layers.background, graphics, camera,
               render_pass);

    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.object);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, object_bind_group, 0,
                                      NULL);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
    layer_draw(&graphics->sprite_layers.background, graphics, camera,
               render_pass);

    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.tilemap);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                      NULL);
    layer_draw(&graphics->tilemap_layers.middle, graphics, camera, render_pass);

    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.object);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, object_bind_group, 0,
                                      NULL);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
    layer_draw(&graphics->sprite_layers.middle, graphics, camera, render_pass);

    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.tilemap);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, tilemap_bind_group, 0,
                                      NULL);
    layer_draw(&graphics->tilemap_layers.foreground, graphics, camera,
               render_pass);

    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.object);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, object_bind_group, 0,
                                      NULL);
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);
    layer_draw(&graphics->sprite_layers.foreground, graphics, camera,
               render_pass);

    wgpuRenderPassEncoderEnd(render_pass);
    wgpuRenderPassEncoderRelease(render_pass);

    WGPURenderPassColorAttachment screen_attachments[] = {{
        .view = frame,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f},
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
    }};
    WGPURenderPassDescriptor screen_render_pass_desc = {
        .label = "screen render pass encoder",
        .colorAttachmentCount = 1,
        .colorAttachments = screen_attachments,
    };
    render_pass = wgpuCommandEncoderBeginRenderPass(command_encoder,
                                                    &screen_render_pass_desc);
    wgpuRenderPassEncoderSetPipeline(render_pass, graphics->shaders.lighting);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, light_bind_group, 0, 0);

    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass, 0, graphics->quad_manager.buffer, 0, quad_buffer_size);

    wgpuRenderPassEncoderDraw(render_pass, VERTICES_PER_QUAD, 1,
                              QUAD_ENTRY_TO_VERTEX_INDEX(screen_quad_index), 0);

    ImGui_ImplWGPU_RenderDrawData(igGetDrawData(), render_pass);

    wgpuRenderPassEncoderEnd(render_pass);

    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, NULL);
    wgpuQueueSubmit(graphics->wgpu.queue, 1, &command_buffer);
    wgpuSurfacePresent(graphics->wgpu.surface);

    wgpuBindGroupRelease(object_bind_group);
    wgpuBindGroupRelease(light_bind_group);
    wgpuBindGroupRelease(tilemap_bind_group);

    wgpuCommandBufferRelease(command_buffer);
    wgpuRenderPassEncoderRelease(render_pass);
    wgpuCommandEncoderRelease(command_encoder);
    wgpuTextureViewRelease(frame);
    wgpuTextureRelease(surface_texture.texture);
}

void graphics_free(Graphics *graphics)
{
    quad_manager_free(&graphics->quad_manager);
    transform_manager_free(&graphics->transform_manager);
    texture_manager_free(&graphics->texture_manager);

    layer_free(&graphics->tilemap_layers.background);
    layer_free(&graphics->tilemap_layers.middle);
    layer_free(&graphics->tilemap_layers.foreground);

    layer_free(&graphics->sprite_layers.background);
    layer_free(&graphics->sprite_layers.middle);
    layer_free(&graphics->sprite_layers.foreground);

    wgpuRenderPipelineRelease(graphics->shaders.object);
    wgpuBindGroupLayoutRelease(graphics->bind_group_layouts.object);

    wgpu_resources_free(&graphics->wgpu);
}

void graphics_resize(Graphics *graphics, int width, int height)
{
    graphics->wgpu.surface_config.width = width;
    graphics->wgpu.surface_config.height = height;
    wgpuSurfaceConfigure(graphics->wgpu.surface,
                         &graphics->wgpu.surface_config);
}
