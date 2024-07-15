#pragma once

#include <stdlib.h>
#include <stdio.h>

#define FATAL(...) fprintf(stderr, __VA_ARGS__), exit(1);

#define SDL_ERRCHK(expr, msg)                                                  \
    {                                                                          \
        if (expr)                                                              \
        {                                                                      \
            FATAL("SDL error:" msg ": %s\n", SDL_GetError());                  \
        }                                                                      \
    }
#define SDL_PTR_ERRCHK(expr, msg) SDL_ERRCHK(!expr, msg)

#define FMOD_ERRCHK(expr, msg)                                                 \
    {                                                                          \
        if (expr != FMOD_OK)                                                   \
        {                                                                      \
            FATAL("FMOD error: " msg ": %s\n", FMOD_ErrorString(result));      \
        }                                                                      \
    }
#define PTR_ERRCHK(expr, msg)                                                  \
    {                                                                          \
        if (expr == NULL)                                                      \
        {                                                                      \
            FATAL("Error: " msg "\n");                                         \
        }                                                                      \
    }
