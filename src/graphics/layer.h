#pragma once

#include <wgpu.h>
#include "sensible_nums.h"
#include "utility/vec.h"

typedef struct Graphics Graphics;

// called before the layer is drawn.
// this is used to switch the pipeline, bind groups, etc.
typedef void (*prepare_fn)(void *this, Graphics *graphics,
                           WGPURenderPassEncoder pass);
// called to render one thing.
typedef void (*thing_draw_fn)(void *this, Graphics *graphics,
                              WGPURenderPassEncoder pass);
// called when the layer is freed (do we call this when the thing is removed
// though?)
typedef void (*thing_free_fn)(void *this);

// holds a bundle of one type of thing.
// this is so we can minimize the amount of pipeline + bind group changes we
// have to do.
typedef struct
{
    vec entries; // either occupied, or an index to the next free entry
    u32 next;

    prepare_fn prepare;
    thing_draw_fn draw;
    thing_free_fn free;
} Layer;

// we store pointers to things in the layer so a u32 for ENTRY_FREE won't cut
// it, we need SIZE_MAX
#define LAYER_ENTRY_FREE SIZE_MAX
typedef u32 LayerEntry;

void layer_init(Layer *layer, prepare_fn prepare, thing_draw_fn draw,
                thing_free_fn free);
void layer_free(Layer *layer);

LayerEntry layer_add(Layer *layer, void *thing);
void layer_remove(Layer *layer, LayerEntry entry);

void layer_draw(Layer *layer, Graphics *graphics, WGPURenderPassEncoder pass);
