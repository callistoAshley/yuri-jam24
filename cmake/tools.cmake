
set(SHADOWCAST_NAME shadowcast)
set(SHADOWCAST_SOURCES
    tools/shadowcast.c
    src/utility/vec.c
)
add_executable(${SHADOWCAST_NAME} ${SHADOWCAST_SOURCES})
target_link_libraries(${SHADOWCAST_NAME} SDL3_image::SDL3_image SDL3::SDL3)