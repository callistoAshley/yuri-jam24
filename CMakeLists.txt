cmake_minimum_required(VERSION 3.20)
project(game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd
set(CMAKE_BUILD_TYPE Debug) # force debug build for now

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

add_custom_target(wgpu-native ALL
    COMMAND make lib-native-release EXTRA_BUILD_ARGS=--quiet
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/wgpu-native
)

find_package(FMOD) # creates FMOD_LIBRARY, FMOD_INCLUDE_DIR, FMOD_STUDIO_LIBRARY and FMOD_STUDIO_INCLUDE_DIR
find_package(WGPU) # creates WGPU_LIBRARY and WGPU_INCLUDE_DIR

# cimgui
# goofy hack: some internal imgui sources rely on "webgpu/webgpu.h", so create a symlink to it
file(CREATE_LINK ${WGPU_INCLUDE_DIR}/webgpu-headers ${CMAKE_SOURCE_DIR}/webgpu SYMBOLIC)
set(IMGUI_STATIC ON)
add_subdirectory(vendor/cimgui/)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(vendor/box2d/)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tests.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/sdl_renames.cmake)

option(CGLM_SHARED OFF)
option(CGLM_STATIC ON)
add_subdirectory(vendor/cglm/)

add_subdirectory(vendor/accesskit-c)

include_directories(
    include/
    ${FMOD_INCLUDE_DIR}
    ${FMOD_STUDIO_INCLUDE_DIR}
    ${WGPU_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${WGPU_INCLUDE_DIR}/webgpu-headers

    cimgui/
    cimgui/imgui/

    src/
)

set(EXECUTABLE_NAME game)
add_subdirectory(src)
add_executable(${EXECUTABLE_NAME}
    ${SOURCES}
    vendor/cimgui/imgui/backends/imgui_impl_wgpu.cpp
    vendor/cimgui/imgui/backends/imgui_impl_sdl3.cpp
)

add_dependencies(${EXECUTABLE_NAME} wgpu-native)

target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -Wpedantic -fpermissive -Wno-error)

target_link_libraries(
    ${EXECUTABLE_NAME} 
    SDL3::SDL3
    SDL3::Headers
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
    cglm
    ${FMOD_LIBRARY}
    ${FMOD_STUDIO_LIBRARY}
    ${WGPU_LIBRARY}
    box2d
    cimgui
    m # link against math library (looks really stupid lol)
    accesskit
)

