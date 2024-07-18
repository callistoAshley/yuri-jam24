#pragma once

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS 
#include <cimgui.h>

typedef struct Window
{
    void (*init_fn)  (struct Window *self); 
    void (*update_fn)(struct Window *self);
    void (*free_fn)  (struct Window *self);

    void *userdata;
} Window;
