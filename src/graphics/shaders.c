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
    char *source;
    long len;
    read_entire_file("assets/shaders/basic.spv", &source, &len);

    GLuint create_shaders[2];
    create_shaders[0] = glCreateShader(GL_VERTEX_SHADER);
    create_shaders[1] = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderBinary(2, create_shaders, GL_SHADER_BINARY_FORMAT_SPIR_V, source,
                   len);
    free(source);
    glSpecializeShader(create_shaders[0], "vs_main", 0, NULL, NULL);
    glSpecializeShader(create_shaders[1], "fs_main", 0, NULL, NULL);

    shaders->basic = glCreateProgram();

    glAttachShader(shaders->basic, create_shaders[0]);
    glAttachShader(shaders->basic, create_shaders[1]);
    glLinkProgram(shaders->basic);

    glDeleteShader(create_shaders[0]);
    glDeleteShader(create_shaders[1]);
}
