#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"
#include "graphics/graphics.h"
#include "utility/macros.h"
#include "utility/linked-list.h"

typedef struct WindowContainer
{
    // FIXME: give this variable a better name
    void *owner; // a pointer to the top-level structure that created this window container, for example the LevelEditor structure
                 // meaning this will never be a pointer to another WindowContainer
    Graphics *graphics;
    LinkedList *windows;
} WindowContainer;

WindowContainer *wndcont_init(void *owner, Graphics *graphics);
Window          *wndcont_add(WindowContainer *cont, Window window);
void             wndcont_remove(WindowContainer *cont, Window *window);
void             wndcont_update(WindowContainer *cont);
void             wndcont_free(WindowContainer *cont);
