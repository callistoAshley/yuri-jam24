#include <fmod_errors.h>
#include <fmod_studio.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define FATAL(...) return fprintf(stderr, __VA_ARGS__), EXIT_FAILURE;
#define SDL_ERRCHK(expr, msg)                                                  \
  {                                                                            \
    if (expr) {                                                                \
      FATAL("SDL error:" #msg ": %s\n", SDL_GetError());                       \
    }                                                                          \
  }
#define SDL_PTR_ERRCHK(expr, msg) SDL_ERRCHK(!expr, msg)
#define FMOD_ERRCHK(expr, msg)                                                 \
  {                                                                            \
    FMOD_RESULT result = expr;                                                 \
    if (result != FMOD_OK) {                                                   \
      FATAL("FMOD error:" #msg ": %s\n", FMOD_ErrorString(result));            \
    }                                                                          \
  }

int main(int argc, char *argv[]) {
  SDL_GLContext gl_context;
  SDL_Window *window;

  FMOD_STUDIO_SYSTEM *system;
  FMOD_ERRCHK(FMOD_Studio_System_Create(&system, FMOD_VERSION),
              "Creating system");
  FMOD_ERRCHK(FMOD_Studio_System_Initialize(system, 1024,
                                            FMOD_STUDIO_INIT_NORMAL,
                                            FMOD_INIT_NORMAL, NULL),
              "Initializing system");

  FMOD_SYSTEM *coreSystem;
  FMOD_Studio_System_GetCoreSystem(system, &coreSystem);

  unsigned int version;
  FMOD_System_GetVersion(coreSystem, &version, NULL);
  printf("FMOD Version: %d.%d.%d\n", version >> 16, version >> 8 & 0xFF,
         version & 0xFF);

  int result = SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  SDL_ERRCHK(result, "SDL initialization failure")

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

  window = SDL_CreateWindow("i am the window", 640, 480, SDL_WINDOW_OPENGL);
  SDL_PTR_ERRCHK(window, "Window creation failure")

  gl_context = SDL_GL_CreateContext(window);
  SDL_PTR_ERRCHK(gl_context, "GL context creation failure")

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    FATAL("ERROR: GLAD initialization failure.\n");
  }

  result = SDL_GL_MakeCurrent(window, gl_context);
  SDL_ERRCHK(result, "SDL_GL_MakeCurrent failure")

  printf("GL Version: %s\n", glGetString(GL_VERSION));

  while (true) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_ESCAPE)
          exit(EXIT_SUCCESS);
        break;
      case SDL_EVENT_QUIT:
        exit(EXIT_SUCCESS);
        break;
      default:
        break;
      }
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(window);
    SDL_Delay(1 / 60);
  }

  FMOD_ERRCHK(FMOD_Studio_System_Release(system), "Releasing system");

  SDL_Quit();
}
