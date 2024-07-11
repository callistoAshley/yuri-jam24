cmake_minimum_required(VERSION 3.20)
project(game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd
set(CMAKE_BUILD_TYPE Debug) # force debug build for now

find_package(SDL3 REQUIRED)

include_directories(
    glad/include/
)

add_subdirectory(glad)

add_executable(game
    src/main.c
)

target_link_libraries(
    game 
    ${SDL3_LIBRARIES}
    glad
)
