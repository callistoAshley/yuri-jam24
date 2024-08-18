#include "physics/physics.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <fmod_errors.h>
#include <fmod_studio.h>

#include <SDL3/SDL.h>
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
#include "events/interpreter.h"
#include "player.h"

int main(int argc, char **argv)
{
    LevelEditor *level_editor = NULL;
    bool init_level_editor = false;
    bool imgui_demo = false;

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

    Player player = PLAYER_INIT;

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS),
               "SDL initialization failure");

    window = SDL_CreateWindow("i am the window", WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_HIDDEN);
    SDL_PTR_ERRCHK(window, "window creation failure");

    graphics_init(&graphics, window);

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

    u64 last_frame = SDL_GetTicksNS();
    u64 accumulator = 0;

    const u64 FIXED_TIME_STEP = SDL_SECONDS_TO_NS(1) / STEPS_PER_SEC;

    bool fullscreen = false;
    while (!input_is_down(&input, Button_Quit))
    {
        SDL_Event event;

        u64 current_frame = SDL_GetTicksNS();
        u64 delta_time = current_frame - last_frame;
        last_frame = current_frame;
        accumulator += delta_time;

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        if (imgui_demo)
            igShowDemoWindow(NULL);

        input_start_frame(&input);
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
            accumulator -= FIXED_TIME_STEP;
        }

        igRender();
        graphics_render(&graphics, &input);

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
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplWGPU_Shutdown();
    igDestroyContext(imgui);

    return 0;
}
