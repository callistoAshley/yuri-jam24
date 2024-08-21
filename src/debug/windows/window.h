#pragma once

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS 
#include <cimgui.h>

struct WindowContainer;

typedef struct Window
{
    bool remove;

    void (*init_fn)  (struct Window *self); 
    void (*update_fn)(struct Window *self);
    void (*free_fn)  (struct Window *self);

    struct WindowContainer *children; 
    struct WindowContainer *wnd_cont; // the window container to which this window belongs
    void *userdata;
} Window;
