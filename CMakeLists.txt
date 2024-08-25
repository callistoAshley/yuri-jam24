cmake_minimum_required(VERSION 3.20)
project(game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd
set(CMAKE_BUILD_TYPE Debug) # force debug build for now

# FMOD
find_library(
    FMOD_LIBRARY 
    NAMES fmod 
    PATHS ${CMAKE_SOURCE_DIR}/fmod/api/core/lib/x86_64 
    REQUIRED
)
find_path(
    FMOD_INCLUDE_DIR
    NAMES fmod.h
    PATHS ${CMAKE_SOURCE_DIR}/fmod/api/core/inc 
    REQUIRED
)
# FMOD Studio
find_library(
    FMOD_STUDIO_LIBRARY 
    NAMES fmodstudio 
    PATHS ${CMAKE_SOURCE_DIR}/fmod/api/studio/lib/x86_64 
    REQUIRED
)
find_path(
    FMOD_STUDIO_INCLUDE_DIR
    NAMES fmod_studio.h
    PATHS ${CMAKE_SOURCE_DIR}/fmod/api/studio/inc 
    REQUIRED
)

add_custom_target(wgpu-native ALL
    COMMAND make lib-native-release EXTRA_BUILD_ARGS=--quiet
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/wgpu-native
)
find_library(
  WGPU_LIBRARY
  NAMES wgpu_native
  PATHS ${CMAKE_SOURCE_DIR}/vendor/wgpu-native/target/release
  REQUIRED
)
find_path(
  WGPU_INCLUDE_DIR
  NAMES wgpu.h
  PATHS ${CMAKE_SOURCE_DIR}/vendor/wgpu-native/ffi
  REQUIRED
)

# cimgui
# goofy hack: some internal imgui sources rely on "webgpu/webgpu.h", so create a symlink to it
file(CREATE_LINK ${WGPU_INCLUDE_DIR}/webgpu-headers ${CMAKE_SOURCE_DIR}/webgpu SYMBOLIC)
set(IMGUI_STATIC ON)
add_subdirectory(vendor/cimgui/)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(vendor/box2d/)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
add_subdirectory(vendor/SDL)

set(SDLTTF_VENDORED ON)
set(SDLTTF_BUILD_SHARED_LIBS OFF)
add_subdirectory(vendor/SDL_ttf)

option(CGLM_SHARED OFF)
option(CGLM_STATIC ON)
add_subdirectory(vendor/cglm/)

include_directories(
    include/
    ${FMOD_INCLUDE_DIR}
    ${FMOD_STUDIO_INCLUDE_DIR}
    ${WGPU_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${WGPU_INCLUDE_DIR}/webgpu-headers

    stb/
    cimgui/
    cimgui/imgui/

    src/
)

add_executable(game
    src/audio/audio.c

    src/debug/windows/brush.c
    src/debug/windows/new-map.c
    src/debug/windows/test.c
    src/debug/windows/tilemap-editor.c
    src/debug/windows/window-container.c
    src/debug/level-editor.c

    src/events/interpreter.c
    src/events/lexer.c

    src/graphics/graphics.c
    src/graphics/shaders.c
    src/graphics/tex_manager.c
    src/graphics/quad_manager.c
    src/graphics/wgpu_resources.c
    src/graphics/bind_group_layouts.c
    src/graphics/binding_helper.c
    src/graphics/transform_manager.c
    src/graphics/tilemap.c
    src/graphics/layer.c
    src/graphics/sprite.c

    src/fonts/font.c

    src/physics/physics.c
    src/physics/debug_draw.c

    src/player.c

    src/input/input.c

    src/utility/linked-list.c
    src/utility/list.c
    src/utility/log.c
    src/utility/vec.c
    src/utility/graphics.c

    stb/stb_impl.c

    src/core_types.c
    src/lvl-parser.c

    src/main.c

    vendor/cimgui/imgui/backends/imgui_impl_wgpu.cpp
    vendor/cimgui/imgui/backends/imgui_impl_sdl3.cpp
)

enable_testing()
add_executable(vec_test         tests/vec_test.c         src/utility/vec.c)
add_executable(linked_list_test tests/linked_list_test.c src/utility/linked-list.c)
add_test(NAME vec_test         COMMAND $<TARGET_FILE:vec_test>)
add_test(NAME linked_list_test COMMAND $<TARGET_FILE:linked_list_test>)

add_dependencies(game wgpu-native)

target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic -fpermissive)

target_link_libraries(
  game 
  SDL3::SDL3
  SDL3::Headers
  SDL3_ttf::SDL3_ttf
  cglm
  ${FMOD_LIBRARY}
  ${FMOD_STUDIO_LIBRARY}
  ${WGPU_LIBRARY}
  box2d
  cimgui
  m # link against math library (looks really stupid lol)
)

include(${CMAKE_SOURCE_DIR}/cmake/sdl_renames.cmake)
