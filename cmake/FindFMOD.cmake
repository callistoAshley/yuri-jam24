# FMOD
find_library(
    FMOD_LIBRARY 
    NAMES fmod 
    PATHS ${CMAKE_SOURCE_DIR}/vendor/fmod/api/core/lib/x86_64 
    REQUIRED
)
find_path(
    FMOD_INCLUDE_DIR
    NAMES fmod.h
    PATHS ${CMAKE_SOURCE_DIR}/vendor/fmod/api/core/inc 
    REQUIRED
)
# FMOD Studio
find_library(
    FMOD_STUDIO_LIBRARY 
    NAMES fmodstudio 
    PATHS ${CMAKE_SOURCE_DIR}/vendor/fmod/api/studio/lib/x86_64 
    REQUIRED
)
find_path(
    FMOD_STUDIO_INCLUDE_DIR
    NAMES fmod_studio.h
    PATHS ${CMAKE_SOURCE_DIR}/vendor/fmod/api/studio/inc 
    REQUIRED
)
