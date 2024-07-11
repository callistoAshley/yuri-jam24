cmake_minimum_required(VERSION 3.20)
project(game)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(game
    src/main.c
)

target_link_libraries(game ${SDL3_LIBRARIES})
target_link_libraries(game Vulkan::Vulkan)
