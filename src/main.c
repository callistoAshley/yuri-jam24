#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "scenes/title.h"
#include "settings.h"
#include "webgpu.h"
#include <fmod_errors.h>
#include <fmod_studio.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include "graphics/imgui_wgpu.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio/audio.h"
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
#include "debug/debug_window.h"

#define WINDOW_NAME "i am the window"

int main(int argc, char **argv)
{
    bool imgui_demo = false;
    bool debug = false;

    for (int i = 0; i < argc; i++)
    {
        imgui_demo |= !strcmp(argv[i], "--imgui-demo");
        debug |= !strcmp(argv[i], "--debug");
    }

    SDL_Window *window;
    bool first_frame = true;

    // Graphics stuff
    Graphics graphics;

    // audio things
    Audio audio;
    audio_init(&audio, debug);

    Input input;
    input_init(&input);

    Physics physics;
    physics_init(&physics);

    Camera raw_camera = {
        .x = 0,
        .y = 0,
        .z = 0,
    };

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS),
               "SDL initialization failure");

    SDL_ERRCHK(TTF_Init(), "TTF initialization failure");

    SDL_ERRCHK(IMG_Init(IMG_INIT_PNG) == 0, "IMG initialization failure");

    Fonts fonts;
    fonts_init(&fonts);

    const char *pref_path =
        SDL_GetPrefPath("callistoAshley", "transbian-god-conquerer");
    const char *settings_name = "settings.ini";
    char *settings_path = malloc(strlen(pref_path) + strlen(settings_name) + 1);
    strcpy(settings_path, pref_path);
    strcat(settings_path, settings_name);

    printf("settings path: %s\n", settings_path);

    Settings settings;
    settings_load_from(&settings, settings_path);

    SDL_WindowFlags flags = SDL_WINDOW_HIDDEN;
    if (settings.video.fullscreen)
        flags = SDL_WINDOW_FULLSCREEN;

    window = SDL_CreateWindow(WINDOW_NAME, WINDOW_WIDTH, WINDOW_HEIGHT, flags);
    SDL_PTR_ERRCHK(window, "window creation failure");

    if (settings.video.fullscreen)
        SDL_SyncWindow(window);

    graphics_init(&graphics, window, &settings);

    // graphics_init may have edited settings, so we need to save them again
    settings_save_to(&settings, settings_path);

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

    Scene *scene_data;
    SceneInterface scene = TITLE_SCENE;

    Resources resources = {
        .debug_mode = debug,
        .graphics = &graphics,
        .physics = &physics,
        .audio = &audio,
        .input = &input,
        .fonts = &fonts,
        .settings = &settings,
        .raw_camera = &raw_camera,
        .current_scene = &scene_data,
        .current_scene_interface = &scene,
        .window = window,
    };

    scene.init(&scene_data, &resources, NULL);

    DebugWindowState dbg_wnd = {
        .resources = &resources,
    };

    u64 accumulator = 0;
    const u64 FIXED_TIME_STEP = SDL_SECONDS_TO_NS(1) / FIXED_STEPS_PER_SEC;

    while (!input_is_down(&input, Button_Quit) && !input.requested_quit)
    {
        SDL_Event event;

        input_start_frame(&input);
        accumulator += input.delta;

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        if (imgui_demo)
            igShowDemoWindow(NULL);

        if (debug)
            debug_wnd_show(&dbg_wnd);

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            input_process(&event, &input, &settings);

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                graphics_resize(&graphics, event.window.data1,
                                event.window.data2);
            }
        }

        if (input_is_pressed(&input, Button_Fullscreen))
        {
            settings.video.fullscreen = !settings.video.fullscreen;
            SDL_SetWindowFullscreen(window, settings.video.fullscreen);
        }

        FMOD_Studio_System_Update(audio.system);

        while (accumulator >= FIXED_TIME_STEP)
        {
            physics_update(&physics);
            if (scene.fixed_update)
                scene.fixed_update(scene_data, &resources);
            accumulator -= FIXED_TIME_STEP;
        }

        scene.update(scene_data, &resources);

        igRender();
        graphics_render(&graphics, &physics, raw_camera);

        if (first_frame)
        {
            SDL_ShowWindow(window);
            first_frame = false;
        }

        // if the frame cap is enabled, and we've got a non-vsync present mode,
        // block until the next frame
        bool framecap_enabled =
            settings.video.frame_cap &&
            (settings.video.present_mode == WGPUPresentMode_Immediate ||
             settings.video.present_mode == WGPUPresentMode_Mailbox);
        if (framecap_enabled)
        {
            f32 frame_time = 1.0 / settings.video.max_framerate;

            u64 now = SDL_GetTicksNS();
            u64 delta = now - input.last_frame;
            f32 delta_seconds = SDL_NS_TO_SECONDS((f32)delta);

            f32 sleep_time = frame_time - delta_seconds;
            if (sleep_time > 0.0)
            {
                SDL_Delay(sleep_time * 1000);
            }
        }
    }

    scene.free(scene_data, &resources);

    settings_save_to(&settings, settings_path);

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplWGPU_Shutdown();
    igDestroyContext(imgui);

    fonts_free(&fonts);
    physics_free(&physics);
    graphics_free(&graphics);
    audio_free(&audio);
    interpreter_free(interpreter);

    SDL_DestroyWindow(window);

    SDL_Quit();
    IMG_Quit();
    TTF_Quit();

    return 0;
}
