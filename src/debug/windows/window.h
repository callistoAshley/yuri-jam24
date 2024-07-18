#pragma once

typedef struct Window
{
    void (*init_fn)  (struct Window *self); 
    void (*update_fn)(struct Window *self);
    void (*free_fn)  (struct Window *self);

    void *userdata;
} Window;
