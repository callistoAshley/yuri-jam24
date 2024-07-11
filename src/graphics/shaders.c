#include "shaders.h"
#include "utility/macros.h"

#include <stdio.h>

GLuint compile_shader(const char *source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        FATAL("ERROR: Shader compilation failure: %s\n", info_log);
    }

    return shader;
}

GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        FATAL("ERROR: Program linking failure: %s\n", info_log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

char *read_entire_file(const char *path)
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
    return buffer;
}

void shaders_init(Shaders *shaders)
{
    char *vertex_source = read_entire_file("assets/shaders/basic.vert");
    GLuint vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    free(vertex_source);

    char *fragment_source = read_entire_file("assets/shaders/basic.frag");
    GLuint fragment_shader =
        compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    free(fragment_source);

    shaders->basic = link_program(vertex_shader, fragment_shader);
}
