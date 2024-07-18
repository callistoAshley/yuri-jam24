#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"
#include "utility/macros.h"
#include "utility/vec.h"

typedef struct
{
    vec windows;
} WindowContainer;

WindowContainer *wndcont_init(void);
Window          *wndcont_add(WindowContainer *cont, Window window);
void             wndcont_remove(WindowContainer *cont, Window *window);
void             wndcont_update(WindowContainer *cont);
void             wndcont_free(WindowContainer *cont);
