#pragma once

#include <wgpu.h>
#include "sensible_nums.h"
#include "utility/vec.h"

typedef struct Graphics Graphics;

typedef struct
{
    vec entries; // either occupied, or an index to the next free entry
    u32 next;
} Layer;

// we store pointers to things in the layer so a u32 for ENTRY_FREE won't cut
// it, we need SIZE_MAX
#define LAYER_ENTRY_FREE SIZE_MAX
typedef u32 LayerEntry;

typedef void (*thing_draw_fn)(void *this, Graphics *graphics,
                              WGPURenderPassEncoder pass);
// called when the layer is freed (do we call this when the thing is removed
// though?)
typedef void (*thing_free_fn)(void *this);

void layer_init(Layer *layer);
void layer_free(Layer *layer);

LayerEntry layer_add(Layer *layer, void *thing, thing_draw_fn draw,
                     thing_free_fn free);
void layer_remove(Layer *layer, LayerEntry entry);

void layer_draw(Layer *layer, Graphics *graphics, WGPURenderPassEncoder pass);
