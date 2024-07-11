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
)

add_executable(game
    src/events/command-funcs.c
    src/events/command-list.c
    src/events/event.c
    src/events/interpreter.c

    src/utility/list.c

    src/main.c
)

target_link_libraries(
    game 
    ${SDL3_LIBRARIES}
    glad
    ${FMOD_LIBRARY}
    ${FMOD_STUDIO_LIBRARY}
    ${LUA_LIBRARY}
)
