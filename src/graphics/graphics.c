#include <glad/gl.h>
#include <stdio.h>

#include <SDL3/SDL_opengl.h>

#include "graphics.h"
#include "utility/macros.h"

GLuint VBO;
GLuint VAO;

GLfloat vertices[] = {
    // 1st
    0.0f, 0.5f, 0.0f,
    // color
    1.0f, 0.0f, 0.0f,
    // 2nd
    -0.5f, -0.5f, 0.0f,
    // color
    0.0f, 1.0f, 0.0f,
    // 3rd
    0.5f, -0.5f, 0.0f,
    // color
    0.0f, 0.0f, 1.0f};

void graphics_init(Graphics *graphics, SDL_Window *window)
{
    graphics->ctx = SDL_GL_CreateContext(window);
    SDL_PTR_ERRCHK(graphics->ctx, "GL context creation failure")

    if (!gladLoadGL(SDL_GL_GetProcAddress))
    {
        FATAL("ERROR: GLAD initialization failure.\n");
    }

    SDL_ERRCHK(SDL_GL_MakeCurrent(window, graphics->ctx),
               "SDL_GL_MakeCurrent failure");

    // should expect 4.6.0 (or similar)
    printf("OpenGL %s\n", glGetString(GL_VERSION));

    shaders_init(&graphics->shaders);

    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);

    glNamedBufferData(VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 6 * sizeof(GLfloat));
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 0, 0);

    glEnableVertexArrayAttrib(VAO, 1);
    glVertexArrayVertexBuffer(VAO, 1, VBO, 3 * sizeof(GLfloat),
                              6 * sizeof(GLfloat));
    glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 1, 1);
}

void graphics_render(Graphics *graphics, Player *player)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(graphics->shaders.basic);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
