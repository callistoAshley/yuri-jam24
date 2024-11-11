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

#include "utility/macros.h"
#include "utility/common_defines.h"
#include "debug/debug_window.h"
#include "events/compiler.h"
#include "scenes/fmod_logo.h"
#include "scenes/title.h"
#include "settings.h"
#include "utility/files.h"

#define WINDOW_NAME "i am the window"

static void event_free_fn(usize i, void *ptr)
{
    (void)i;
    Event *event = ptr;
    event_free(event);
}

int main(int argc, char **argv)
{
    bool imgui_demo = false;
    bool debug = false;

    for (int i = 0; i < argc; i++)
    {
        imgui_demo |= !strcmp(argv[i], "--imgui-demo");
        debug |= !strcmp(argv[i], "--debug");
    }

    Resources resources = {0};
    bool first_frame = true;

    resources.raw_camera = (Camera){
        .x = 0,
        .y = 0,
        .z = 0,
    };

    SDL_ERRCHK(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS),
               "SDL initialization failure");

    SDL_ERRCHK(TTF_Init(), "TTF initialization failure");

    SDL_ERRCHK(IMG_Init(IMG_INIT_PNG) == 0, "IMG initialization failure");

    resources.window = SDL_CreateWindow(WINDOW_NAME, BASE_WINDOW_WIDTH,
                                        BASE_WINDOW_HEIGHT, SDL_WINDOW_HIDDEN);
    SDL_PTR_ERRCHK(resources.window, "window creation failure");

    SDL_DisplayID display = SDL_GetDisplayForWindow(resources.window);
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);

    const char *pref_path =
        SDL_GetPrefPath("callistoAshley", "transbian-god-conquerer");
    const char *settings_name = "settings.ini";
    char *settings_path = malloc(strlen(pref_path) + strlen(settings_name) + 1);
    strcpy(settings_path, pref_path);
    strcat(settings_path, settings_name);

    printf("settings path: %s\n", settings_path);

    settings_load_from(&resources.settings, mode->refresh_rate, settings_path);
    resources.settings.debug = debug;

    audio_init(&resources.audio, debug, &resources.settings);
    input_init(&resources.input, resources.window);
    graphics_init(&resources.graphics, resources.window, &resources.settings);
    physics_init(&resources.physics);
    fonts_init(&resources.fonts);

    // graphics_init may have edited settings, so we need to save them again
    settings_save_to(&resources.settings, settings_path);

    char *files[] = {
        "assets/events.txt",
    };
    vec events;
    vec_init(&events, sizeof(Event));

    for (u32 i = 0; i < 1; i++)
    {
        char *out;
        read_entire_file(files[i], &out, NULL);

        Compiler compiler;
        compiler_init(&compiler, out);

        Event event;
        while (compiler_compile(&compiler, &event))
        {
            event_disassemble(&event);
            vec_push(&events, &event);
        }

        free(out);
    }

    resources.events = (Event *)events.data;
    resources.event_count = events.len;

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
        .Device = resources.graphics.wgpu.device,
        .NumFramesInFlight = 3,
        .PipelineMultisampleState = multisample_state,
        .RenderTargetFormat = resources.graphics.wgpu.surface_config.format,
    };
    ImGui_ImplSDL3_InitForOther(resources.window);
    ImGui_ImplWGPU_Init(&imgui_init_info);

    resources.time.real = time_real_new();
    resources.time.virt = time_virt_new();
    resources.time.fixed = time_fixed_new();

    if (resources.settings.debug)
        resources.scene_interface = TITLE_SCENE;
    else
        resources.scene_interface = FMOD_LOGO_SCENE;

    resources.scene_interface.init(&resources, NULL);

    DebugWindowState dbg_wnd = {
        .resources = &resources,
    };

    while (!input_is_down(&resources.input, Button_Quit) &&
           !resources.input.requested_quit)
    {
        SDL_Event event;

        input_start_frame(&resources.input);

        // update real, fixed, and virtual time
        time_real_update(&resources.time.real);
        time_virt_advance_with(&resources.time.virt,
                               resources.time.real.time.delta);
        time_fixed_accumulate(&resources.time.fixed,
                              resources.time.virt.time.delta);

        Instant before_logic = instant_now();

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            input_process(&resources.input, &event, &resources.settings);

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                graphics_resize(&resources.graphics, event.window.data1,
                                event.window.data2);
            }
        }

        // we have to start the frame after we hand imgui all the events,
        // otherwise imgui will lag 1 frame behind the game logic. this is
        // especially important if the window is resized!
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        if (imgui_demo)
            igShowDemoWindow(NULL);

        if (debug)
            debug_wnd_show(&dbg_wnd);

        if (input_is_pressed(&resources.input, Button_Fullscreen))
        {
            resources.settings.video.fullscreen =
                !resources.settings.video.fullscreen;
            SDL_SetWindowFullscreen(resources.window,
                                    resources.settings.video.fullscreen);
        }

        FMOD_Studio_System_Update(resources.audio.system);

        // preform accumulated fixed updates
        while (time_fixed_expend(&resources.time.fixed))
        {
            resources.time.current = resources.time.fixed.time;
            physics_update(&resources.physics, resources.time.fixed.time);
            if (resources.scene_interface.fixed_update)
                resources.scene_interface.fixed_update(&resources);
        }

        resources.time.current = resources.time.virt.time;
        resources.scene_interface.update(&resources);

        igRender();
        graphics_render(&resources.graphics, &resources.physics,
                        resources.raw_camera);

        if (first_frame)
        {
            if (resources.settings.video.fullscreen)
            {
                SDL_SetWindowFullscreen(resources.window, true);
            }
            SDL_ShowWindow(resources.window);
            SDL_SyncWindow(resources.window);
            first_frame = false;
        }

        Instant after_logic = instant_now();
        Duration logic_time = instant_duration_since(after_logic, before_logic);
        f64 logic_delta = duration_as_secs_f64(logic_time);

        // if the frame cap is enabled,
        // block until the next frame
        if (resources.settings.video.frame_cap)
        {
            f64 frame_time = 1.0 / resources.settings.video.max_framerate;

            f64 sleep_time = frame_time - logic_delta;
            if (sleep_time > 0.0)
            {
                SDL_DelayNS(sleep_time * SDL_NS_PER_SECOND);
            }
        }
    }

    resources.scene_interface.free(&resources);

    vec_free_with(&events, event_free_fn);

    settings_save_to(&resources.settings, settings_path);

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplWGPU_Shutdown();
    igDestroyContext(imgui);

    fonts_free(&resources.fonts);
    physics_free(&resources.physics);
    graphics_free(&resources.graphics);
    audio_free(&resources.audio);

    SDL_DestroyWindow(resources.window);

    SDL_Quit();
    IMG_Quit();
    TTF_Quit();

    return 0;
}
