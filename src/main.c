#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "events/lexer.h"
#include "scenes/title.h"
#include "settings.h"
#include "time/fixed.h"
#include "time/real.h"
#include "time/virt.h"
#include "utility/files.h"
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
#include "player.h"
#include "physics/physics.h"
#include "scenes/fmod_logo.h"
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

    // audio things
    Audio audio;
    audio_init(&audio, debug, &settings);

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

    char *out;
    read_entire_file(files[0], &out, NULL);

    Lexer lexer;
    lexer_init(&lexer, out);

    Token token;
    while (lexer_next(&lexer, &token))
    {
        switch (token.type)
        {
        case Token_Event:
            printf("Event");
            break;
        case Token_Goto:
            printf("Goto");
            break;
        case Token_If:
            printf("If");
            break;
        case Token_Loop:
            printf("Loop");
            break;
        case Token_Op:
        {
            printf("Op ");
            switch (token.data.op)
            {
            case Op_Set:
                printf("Set");
                break;
            case Op_Equals:
                printf("Equals");
                break;
            case Op_NotEq:
                printf("Not Eq");
                break;
            case Op_Less:
                printf("Less");
                break;
            case Op_LessEq:
                printf("Less Eq");
                break;
            case Op_Greater:
                printf("Greater");
                break;
            case Op_GreaterEq:
                printf("Greater Eq");
                break;
            case Op_Not:
                printf("Not");
                break;
            case Op_And:
                printf("And");
                break;
            case Op_Or:
                printf("Or");
                break;
            case Op_Plus:
                printf("Plus");
                break;
            case Op_Minus:
                printf("Minus");
                break;
            case Op_Mult:
                printf("Mult");
                break;
            case Op_Div:
                printf("Div");
                break;
            case Op_Mod:
                printf("Mod");
                break;
            }
            break;
        }
        case Token_None:
            printf("None");
            break;
        case Token_True:
            printf("True");
            break;
        case Token_False:
            printf("False");
            break;
        case Token_BraceL:
            printf("Brace L");
            break;
        case Token_BraceR:
            printf("Brace R");
            break;
        case Token_ParenL:
            printf("Paren L");
            break;
        case Token_ParenR:
            printf("Paren R");
            break;
        case Token_Comma:
            printf("Comma");
            break;
        case Token_Ident:
            printf("Ident: %s", token.data.ident);
            break;
        case Token_Label:
            printf("Label: %s", token.data.label);
            break;
        case Token_String:
            printf("String: %s", token.data.string);
            break;
        case Token_Int:
            printf("Int: %d", token.data._int);
            break;
        case Token_Float:
            printf("Float: %f", token.data._float);
            break;
        }
        printf("\n");
    }
    return 0;

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
    SceneInterface scene = FMOD_LOGO_SCENE;

    TimeReal real = time_real_new();
    TimeVirt virt = time_virt_new();
    TimeFixed fixed = time_fixed_new();

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

        .time.real = &real,
        .time.virt = &virt,
        .time.fixed = &fixed,
    };

    scene.init(&scene_data, &resources, NULL);

    DebugWindowState dbg_wnd = {
        .resources = &resources,
    };

    while (!input_is_down(&input, Button_Quit) && !input.requested_quit)
    {
        SDL_Event event;

        input_start_frame(&input);

        // update real, fixed, and virtual time
        time_real_update(&real);
        time_virt_advance_with(&virt, real.time.delta);
        time_fixed_accumulate(&fixed, virt.time.delta);

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

        if (input_is_pressed(&input, Button_Fullscreen))
        {
            settings.video.fullscreen = !settings.video.fullscreen;
            SDL_SetWindowFullscreen(window, settings.video.fullscreen);
        }

        FMOD_Studio_System_Update(audio.system);

        // preform accumulated fixed updates
        while (time_fixed_expend(&fixed))
        {
            resources.time.current = fixed.time;
            physics_update(&physics, fixed.time);
            if (scene.fixed_update)
                scene.fixed_update(scene_data, &resources);
        }

        resources.time.current = virt.time;
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

            f32 delta_seconds = time_delta_seconds(real.time);

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

    SDL_DestroyWindow(window);

    SDL_Quit();
    IMG_Quit();
    TTF_Quit();

    return 0;
}
