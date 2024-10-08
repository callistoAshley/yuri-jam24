#pragma once

#include <wgpu.h>
#include "sensible_nums.h"
#include "utility/vec.h"
#include "core_types.h"

typedef struct Graphics Graphics;

// called to render one thing.
typedef void (*thing_draw_fn)(void *this, void *ctx,
                              WGPURenderPassEncoder pass);

// holds a bundle of one type of thing.
// this is so we can minimize the amount of pipeline + bind group changes we
// have to do.
typedef struct
{
    vec entries; // either occupied, or an index to the next free entry
    u32 next;

    thing_draw_fn draw;
} Layer;

// we store pointers to things in the layer so a u32 for ENTRY_FREE won't cut
// it, we need SIZE_MAX
#define LAYER_ENTRY_FREE SIZE_MAX
typedef u32 LayerEntry;

void layer_init(Layer *layer, thing_draw_fn draw);
void layer_free(Layer *layer);

LayerEntry layer_add(Layer *layer, void *thing);
void layer_remove(Layer *layer, LayerEntry entry);

void layer_draw(Layer *layer, void *ctx, WGPURenderPassEncoder pass);
