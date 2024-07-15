#include "shaders.h"
#include "utility/macros.h"

#include <stdio.h>

void read_entire_file(const char *path, char **out, long *len)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        FATAL("ERROR: Could not open file %s\n", path);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);

    *out = buffer;
    *len = length;
}

void shaders_init(Shaders *shaders)
{
    // TODO
}
