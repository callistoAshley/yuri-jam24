#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"
#include "graphics/graphics.h"
#include "utility/macros.h"
#include "utility/vec.h"

typedef struct
{
    void *owner; // a pointer to the structure that created this window container
    Graphics *graphics;
    vec windows;
} WindowContainer;

WindowContainer *wndcont_init(void *owner, Graphics *graphics);
Window          *wndcont_add(WindowContainer *cont, Window window);
void             wndcont_remove(WindowContainer *cont, Window *window);
void             wndcont_update(WindowContainer *cont);
void             wndcont_free(WindowContainer *cont);
