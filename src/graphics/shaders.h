#pragma once

#include "utility/list.h"
#include <glad/gl.h>

typedef struct
{
    GLuint basic;
} Shaders;

void shaders_init(Shaders *shaders);
