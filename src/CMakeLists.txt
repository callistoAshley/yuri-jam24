add_subdirectory(audio)
add_subdirectory(debug)
add_subdirectory(events)
add_subdirectory(fonts)
add_subdirectory(graphics)
add_subdirectory(input)
add_subdirectory(parsers)
add_subdirectory(physics)
add_subdirectory(scenes)
add_subdirectory(utility)

set(SOURCES 
    src/core_types.c 
    src/main.c 
    src/player.c
    ${SOURCES}
    PARENT_SCOPE
)
