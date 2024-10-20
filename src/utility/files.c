#include "files.h"
#include <SDL3/SDL.h>
#include "sensible_nums.h"
#include <stdlib.h>
#include <string.h>

void read_entire_file(const char *path, char **out, long *len)
{
    size_t size = 0;
    void *data = SDL_LoadFile(path, &size);

    *out = malloc(size + 1);
    if (len)
        *len = size;
    (*out)[size] = '\0';

    memcpy(*out, data, size);
    SDL_free(data);
}
