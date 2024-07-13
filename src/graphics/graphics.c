#include <glad/gl.h>
#include <stdio.h>

#include <SDL3/SDL_opengl.h>

#include "graphics.h"
#include "utility/macros.h"

#include <stb_easy_font.h>

GLuint VBO;
GLuint EBO;
GLuint VAO;
unsigned int vertex_count;

#define CHAR_QUAD_SIZE 270

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

    GLuint textures[4];
    glCreateTextures(GL_TEXTURE_2D, 4, textures);
    for (int i = 0; i < 4; i++)
    {
        glTextureStorage2D(textures[i], 1, GL_RGBA8, 1, 1);
        GLuint64 handle = glad_glGetTextureHandleARB(textures[i]);
        glMakeTextureHandleResidentARB(handle);
        printf("Texture handle: %lu\n", handle);
    }

    // create vertex buffer for rendering text
    char *text = "Hello, world!";
    int buf_size = CHAR_QUAD_SIZE * strlen(text);
    void *text_buf = malloc(buf_size);

    unsigned char color[4] = {255, 0, 128, 255};
    int quad_count =
        stb_easy_font_print(0.0f, 0.0f, text, color, text_buf, buf_size);
    vertex_count = quad_count * 6;

    unsigned int *indicies = malloc(quad_count * 6 * sizeof(unsigned int));
    for (int i = 0; i < quad_count; i++)
    {
        indicies[i * 6 + 0] = i * 4 + 0;
        indicies[i * 6 + 1] = i * 4 + 1;
        indicies[i * 6 + 2] = i * 4 + 2;
        indicies[i * 6 + 3] = i * 4 + 2;
        indicies[i * 6 + 4] = i * 4 + 3;
        indicies[i * 6 + 5] = i * 4 + 0;
    }

    glCreateVertexArrays(1, &VAO);

    glCreateBuffers(1, &VBO);
    glCreateBuffers(1, &EBO);

    glNamedBufferData(VBO, buf_size, text_buf, GL_STATIC_DRAW);
    glNamedBufferData(EBO, quad_count * 6 * sizeof(unsigned int), indicies,
                      GL_STATIC_DRAW);

    free(text_buf);
    free(indicies);

    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 4 * sizeof(float));
    glVertexArrayAttribFormat(VAO, 0, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 0, 0);
}

void graphics_render(Graphics *graphics, Player *player)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(graphics->shaders.basic);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, vertex_count, GL_UNSIGNED_INT, 0);
}
