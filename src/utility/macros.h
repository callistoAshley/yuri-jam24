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
            FATAL("FMOD error: " msg ": %s\n", FMOD_ErrorString(expr));        \
        }                                                                      \
    }
#define PTR_ERRCHK(expr, msg)                                                  \
    {                                                                          \
        if (expr == NULL)                                                      \
        {                                                                      \
            FATAL("Error: " msg "\n");                                         \
        }                                                                      \
    }
#define INV_PTR_ERRCHK(expr, msg)                                              \
    {                                                                          \
        if (expr != NULL)                                                      \
        {                                                                      \
            FATAL("Error: " msg "\n");                                         \
        }                                                                      \
    }

#define REALLOC_CHK(temp_ptr, assignee)                                        \
    {                                                                          \
        if (!temp_ptr)                                                         \
        {                                                                      \
            if (assignee)                                                      \
                free(assignee);                                                \
            FATAL("REALLOC_CHK failed at %s:%d.\n", __FILE__, __LINE__);       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            assignee = temp_ptr;                                               \
        }                                                                      \
    }

#define REMAINDER_OF(val, divisor) ((val) % (divisor))
#define ALIGN_TO(val, align)                                                   \
    REMAINDER_OF(val, align) == 0 ? val : val + align - REMAINDER_OF(val, align)

// NOTE: If neither of these are present, the parameter is required to be
// initialized, and non-NULL!

// indicates that this function parameter can be NULL
#define NULLABLE
// indicates that this function parameter can be uninitialized
#define UNINIT

#define STREQ(a, b) !strcmp(a, b)
#define STRNEQ(a, b) strcmp(a, b)
