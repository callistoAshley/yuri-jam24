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

add_subdirectory(glad)

set(CGLM_SHARED OFF)
set(CGLM_STATIC ON)
add_subdirectory(cglm/)

add_custom_target(lua ALL
    command make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lua
)
find_library(
    LUA_LIBRARY
    NAMES lua
    PATHS ${CMAKE_SOURCE_DIR}/lua
    REQUIRED
)

include_directories(
    glad/include/
    include/
    ${FMOD_INCLUDE_DIR}
    ${FMOD_STUDIO_INCLUDE_DIR}
    lua/
    stb/

    src/
)

add_executable(game
    src/events/command-funcs.c
    src/events/command-list.c
    src/events/event.c
    src/events/interpreter.c

    src/audio/audio.c

    src/graphics/graphics.c
    src/graphics/shaders.c

    src/input/input.c

    src/utility/linked-list.c
    src/utility/list.c

    stb/stb_impl.c

    src/main.c
)

target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(
  game 
  ${SDL3_LIBRARIES}
  glad
  cglm
  ${FMOD_LIBRARY}
  ${FMOD_STUDIO_LIBRARY}
  ${LUA_LIBRARY}
  m # link against math library (looks really stupid lol)
)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)

# Shader compilation with naga
find_program(naga_executable NAMES naga PATHS ~/.cargo/bin REQUIRED) 
file(GLOB SHADERS ${SHADER_SOURCE_DIR}/*.wgsl) # find all webgpu shaders

foreach(source ${SHADERS})
  get_filename_component(name ${source} NAME_WE)

  message(STATUS "Building shader" ${source})
  message(STATUS ${naga_executable} ${source} ${SHADER_OUTPUT_DIR}/${name}.spv)

  add_custom_command(
    OUTPUT ${SHADER_SOURCE_DIR}/${name}.spv
    COMMAND ${naga_executable} ${source} ${SHADER_OUTPUT_DIR}/${name}.spv 
    DEPENDS ${source}
  )
  list(APPEND SPV_SHADERS ${SHADER_OUTPUT_DIR}/${name}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})
