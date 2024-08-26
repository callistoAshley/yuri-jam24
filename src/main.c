#include <fmod_errors.h>
#include <fmod_studio.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include "graphics/imgui-wgpu.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio/audio.h"
#include "debug/level-editor.h"
#include "graphics/graphics.h"
#include "input/input.h"
#include "utility/log.h"
#include "utility/macros.h"
#include "utility/common_defines.h"
#include "events/interpreter.h"
#include "player.h"
#include "physics/physics.h"
#include "scenes/map.h"
#include "scenes/scene.h"

int main(int argc, char **argv)
{
    LevelEditor *level_editor = NULL;
    bool init_level_editor = false;
    bool imgui_demo = false;
    bool physics_debug_draw = false;

    for (int i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "--level-editor"))
        {
            init_level_editor = true;
        }
        else if (!strcmp(argv[i], "--imgui-demo"))
        {
            imgui_demo = true;
        }
        else if (!strcmp(argv[i], "--physics-debug-draw"))
        {
            physics_debug_draw = true;
        }
    }

    SDL_Window *window;
    bool first_frame = true;

    // Graphics stuff
    Graphics graphics;

    // audio things
    Audio audio;
    audio_init(&audio);

    Input input;
    input_new(&input);

    Physics physics;
    physics_init(&physics);
    physics.debug_draw = physics_debug_draw;

    Camera raw_camera = {
        .x = 0,
        .y = 0,
        .z = 0,
    };

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS),
               "SDL initialization failure");

    SDL_ERRCHK(TTF_Init(), "TTF initialization failure");

    window = SDL_CreateWindow("i am the window", WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_HIDDEN);
    SDL_PTR_ERRCHK(window, "window creation failure");

    // SDL_SetWindowRelativeMouseMode(window, true);

    graphics_init(&graphics, window);

    Rect tex_coords = rect_from_min_size(GLMS_VEC2_ZERO, GLMS_VEC2_ONE);
    Rect rect = rect_from_min_size(GLMS_VEC2_ZERO, (vec2s){.x = 32., .y = 32.});
    Quad quad = {.rect = rect, .tex_coords = tex_coords};

    QuadEntry quad_entry = quad_manager_add(&graphics.quad_manager, quad);

    f32 mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    Transform transform =
        transform_from_pos((vec3s){.x = mouse_x, .y = mouse_y, .z = 0});
    TransformEntry transform_entry =
        transform_manager_add(&graphics.transform_manager, transform);

    TextureEntry *texture_entry =
        texture_manager_load(&graphics.texture_manager,
                             "assets/textures/cursor.png", &graphics.wgpu);

    Sprite sprite;
    sprite_init(&sprite, texture_entry, transform_entry, quad_entry);
    layer_add(&graphics.ui_layers.foreground, &sprite);

    if (init_level_editor)
        level_editor = lvledit_init(&graphics);

    char *files[] = {
        "assets/events.txt",
    };
    Interpreter *interpreter = interpreter_init(files, 1);

    WGPUMultisampleState multisample_state = {
        .count = 1,
        .mask = 0xFFFFFFFF,
        .alphaToCoverageEnabled = false,
    };

    // imgui initialization
    ImGuiContext *imgui = igCreateContext(NULL);
    ImGuiIO *io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWGPU_InitInfo imgui_init_info = {
        .Device = graphics.wgpu.device,
        .NumFramesInFlight = 3,
        .PipelineMultisampleState = multisample_state,
        .RenderTargetFormat = graphics.wgpu.surface_config.format,
    };
    ImGui_ImplSDL3_InitForOther(window);
    ImGui_ImplWGPU_Init(&imgui_init_info);

    Resources resources = {
        .graphics = &graphics,
        .physics = &physics,
        .audio = &audio,
        .input = &input,
        .raw_camera = &raw_camera,
    };

    void *scene_data;
    SceneInterface scene = MAP_SCENE;
    scene.init(&scene_data, &resources);

    u64 accumulator = 0;
    const u64 FIXED_TIME_STEP = SDL_SECONDS_TO_NS(1) / FIXED_STEPS_PER_SEC;

    bool fullscreen = false;
    while (!input_is_down(&input, Button_Quit))
    {
        SDL_Event event;

        input_start_frame(&input);
        accumulator += input.delta;

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        if (imgui_demo)
            igShowDemoWindow(NULL);

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            input_process(&event, &input);

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                graphics_resize(&graphics, event.window.data1,
                                event.window.data2);
            }
        }

        if (input_is_pressed(&input, Button_Fullscreen))
        {
            fullscreen = !fullscreen;
            SDL_SetWindowFullscreen(window, fullscreen);
        }

        FMOD_Studio_System_Update(audio.system);

        if (level_editor)
        {
            lvledit_update(level_editor);
            if (level_editor->request_quit)
                break;
        }

        while (accumulator >= FIXED_TIME_STEP)
        {
            physics_update(&physics);
            if (scene.fixed_update)
                scene.fixed_update(scene_data, &resources);
            accumulator -= FIXED_TIME_STEP;
        }

        scene.update(scene_data, &resources);

        SDL_GetMouseState(&mouse_x, &mouse_y);
        transform.position.x = mouse_x;
        transform.position.y = mouse_y;
        transform_manager_update(&graphics.transform_manager, transform_entry,
                                 transform);

        igRender();
        graphics_render(&graphics, &physics, raw_camera);

        if (first_frame)
        {
            SDL_ShowWindow(window);
            first_frame = false;
        }
    }

    graphics_free(&graphics);
    audio_free(&audio);
    interpreter_free(interpreter);
    SDL_Quit();
    TTF_Quit();
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplWGPU_Shutdown();
    igDestroyContext(imgui);

    return 0;
}
