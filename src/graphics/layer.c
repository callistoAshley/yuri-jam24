#include "layer.h"
#include <assert.h>

typedef union
{
    struct
    {
        void *thing;
        thing_draw_fn draw;
        thing_free_fn free;
    } entry;

    struct
    {
        size_t is_free;
        LayerEntry next;
    } next;
} LayerEntryData;

#define INITIAL_BUFFER_CAP 32
#define INITIAL_BUFFER_SIZE sizeof(TransformEntryData) * INITIAL_BUFFER_CAP

void layer_init(Layer *layer)
{
    vec_init_with_capacity(&layer->entries, sizeof(LayerEntryData),
                           INITIAL_BUFFER_CAP);
    layer->next = 0;
}

void layer_free_fn(usize index, void *this)
{
    (void)index;

    LayerEntryData *data = this;
    if (data->next.is_free == LAYER_ENTRY_FREE)
        return;

    if (data->entry.free)
        data->entry.free(data->entry.thing);
}

void layer_free(Layer *layer) { vec_free_with(&layer->entries, layer_free_fn); }

LayerEntry layer_add(Layer *layer, void *thing, thing_draw_fn draw,
                     thing_free_fn free)
{
    LayerEntry key = layer->next;

    if (layer->next == layer->entries.len)
    {
        LayerEntryData entry = {
            .entry = {.thing = thing, .draw = draw, .free = free},
        };

        assert(entry.next.is_free != LAYER_ENTRY_FREE);

        vec_push(&layer->entries, &entry);
        layer->next++;
    }
    else
    {
        LayerEntryData *entry = vec_get(&layer->entries, layer->next);
        assert(entry != NULL);
        assert(entry->next.is_free == LAYER_ENTRY_FREE);

        layer->next = entry->next.next;

        *entry = (LayerEntryData){
            .entry = {.thing = thing, .draw = draw, .free = free},
        };
    }

    return key;
}

void layer_remove(Layer *layer, LayerEntry entry)
{
    LayerEntryData *data = vec_get(&layer->entries, entry);
    assert(data != NULL);
    assert(data->next.is_free != LAYER_ENTRY_FREE);

    if (data->entry.free)
        data->entry.free(data->entry.thing);

    data->next.is_free = LAYER_ENTRY_FREE;
    data->next.next = layer->next;
    layer->next = entry;
}

void layer_draw(Layer *layer, Graphics *graphics, WGPURenderPassEncoder pass)
{
    for (usize i = 0; i < layer->entries.len; i++)
    {
        LayerEntryData *data = vec_get(&layer->entries, i);
        if (data->next.is_free == LAYER_ENTRY_FREE)
            continue;

        data->entry.draw(data->entry.thing, graphics, pass);
    }
}
