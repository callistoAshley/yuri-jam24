// cimgui doesn't contain bindings for imgui's wgpu or sdl3 backends, so this
// header exists in its place
#pragma once
#include <webgpu.h>
#include <SDL3/SDL.h>

#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include <cimgui.h>

typedef struct
{
    WGPUDevice Device;
    int NumFramesInFlight;
    WGPUTextureFormat RenderTargetFormat;
    WGPUTextureFormat DepthStencilFormat;
    WGPUMultisampleState PipelineMultisampleState;
} ImGui_ImplWGPU_InitInfo;
typedef struct ImDrawData ImDrawData;

CIMGUI_API bool ImGui_ImplWGPU_Init(ImGui_ImplWGPU_InitInfo *init_info);
CIMGUI_API void ImGui_ImplWGPU_Shutdown(void);
CIMGUI_API void ImGui_ImplWGPU_NewFrame(void);
CIMGUI_API void
ImGui_ImplWGPU_RenderDrawData(ImDrawData *draw_data,
                              WGPURenderPassEncoder pass_encoder);

// Use if you want to reset your rendering device without losing Dear ImGui
// state.
CIMGUI_API void ImGui_ImplWGPU_InvalidateDeviceObjects(void);
CIMGUI_API bool ImGui_ImplWGPU_CreateDeviceObjects(void);

CIMGUI_API bool ImGui_ImplSDL3_InitForOpenGL(SDL_Window *window,
                                             void *sdl_gl_context);
CIMGUI_API bool ImGui_ImplSDL3_InitForVulkan(SDL_Window *window);
CIMGUI_API bool ImGui_ImplSDL3_InitForD3D(SDL_Window *window);
CIMGUI_API bool ImGui_ImplSDL3_InitForMetal(SDL_Window *window);
CIMGUI_API bool ImGui_ImplSDL3_InitForSDLRenderer(SDL_Window *window,
                                                  SDL_Renderer *renderer);
CIMGUI_API bool ImGui_ImplSDL3_InitForOther(SDL_Window *window);
CIMGUI_API void ImGui_ImplSDL3_Shutdown(void);
CIMGUI_API void ImGui_ImplSDL3_NewFrame(void);
CIMGUI_API bool ImGui_ImplSDL3_ProcessEvent(const SDL_Event *event);

// Gamepad selection automatically starts in AutoFirst mode, picking first
// available SDL_Gamepad. You may override this. When using manual mode, caller
// is responsible for opening/closing gamepad.
enum ImGui_ImplSDL3_GamepadMode
{
    ImGui_ImplSDL3_GamepadMode_AutoFirst,
    ImGui_ImplSDL3_GamepadMode_AutoAll,
    ImGui_ImplSDL3_GamepadMode_Manual
};
typedef enum ImGui_ImplSDL3_GamepadMode ImGui_ImplSDL3_GamepadMode;
CIMGUI_API void
ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode mode,
                              SDL_Gamepad **manual_gamepads_array /* = NULL*/,
                              int manual_gamepads_count /* = -1*/);
