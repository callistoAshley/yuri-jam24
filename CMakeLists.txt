cmake_minimum_required(VERSION 3.20)
project(game)

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "we get an annoying warning in freetype about cmake <3.5 being deprecated" FORCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd

# hack to tell cmake to prefer static libraries over shared ones
if(WIN32)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".dll" ".so")
else()
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

find_package(FMOD) # creates FMOD_LIBRARY, FMOD_INCLUDE_DIR, FMOD_STUDIO_LIBRARY and FMOD_STUDIO_INCLUDE_DIR

set(WGPU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/vendor/wgpu-native/ffi)

# cimgui
# goofy hack: some internal imgui sources rely on "webgpu/webgpu.h", so create a symlink to it
# this doesn't work on windows (complains about some permission thing? so we have to do a different hack instead)
if (WIN32)
    file(COPY ${WGPU_INCLUDE_DIR}/webgpu-headers/ DESTINATION ${CMAKE_SOURCE_DIR}/webgpu)
else()
    file(CREATE_LINK ${WGPU_INCLUDE_DIR}/webgpu-headers ${CMAKE_SOURCE_DIR}/webgpu SYMBOLIC)
endif()
set(IMGUI_STATIC "on" CACHE STRING "this is needed to force cimgui to be static?" FORCE)
add_subdirectory(vendor/cimgui/)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(vendor/box2d/)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tests.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/sdl_renames.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tools.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/wgpu.cmake)

option(CGLM_SHARED OFF)
option(CGLM_STATIC ON)
add_subdirectory(vendor/cglm/)

add_subdirectory(vendor/accesskit-c)

add_subdirectory(vendor/tmx)

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
set_property(TARGET ${EXECUTABLE_NAME} PROPERTY C_STANDARD 23)

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
    wgpu_native
    box2d
    cimgui
    m # link against math library (looks really stupid lol)
    accesskit
    tmx
)

if (WIN32)
  target_link_libraries(
    ${EXECUTABLE_NAME}
    "C:/Windows/System32/d3dcompiler_47.dll"
    "C:/Windows/System32/opengl32.dll"
  )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # add the debug definition to the executable target
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE -DDEBUG)
endif()
