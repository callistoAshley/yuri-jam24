cmake_minimum_required(VERSION 3.20)
project(game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd
set(CMAKE_BUILD_TYPE Debug) # force debug build for now

find_package(SDL3 REQUIRED)

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
    COMMAND make lib-native-release EXTRA_BUILD_ARGS=--quiet -E env RUSTFLAGS=-Awarnings
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/wgpu-native
)
find_library(
  WGPU_LIBRARY
  NAMES wgpu_native
  PATHS ${CMAKE_SOURCE_DIR}/wgpu-native/target/release
  REQUIRED
)
find_path(
  WGPU_INCLUDE_DIR
  NAMES wgpu.h
  PATHS ${CMAKE_SOURCE_DIR}/wgpu-native/ffi
  REQUIRED
)

option(CGLM_SHARED OFF)
option(CGLM_STATIC ON)
add_subdirectory(cglm/)

include_directories(
    include/
    ${FMOD_INCLUDE_DIR}
    ${FMOD_STUDIO_INCLUDE_DIR}
    ${WGPU_INCLUDE_DIR}
    ${WGPU_INCLUDE_DIR}/webgpu-headers
    stb/

    src/
)

add_executable(game
    src/audio/audio.c

    src/graphics/graphics.c
    src/graphics/shaders.c

    src/input/input.c

    src/utility/linked-list.c
    src/utility/list.c

    stb/stb_impl.c

    src/main.c
)

add_dependencies(game wgpu-native)

target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(
  game 
  ${SDL3_LIBRARIES}
  cglm
  ${FMOD_LIBRARY}
  ${FMOD_STUDIO_LIBRARY}
  ${WGPU_LIBRARY}
  m # link against math library (looks really stupid lol)
)