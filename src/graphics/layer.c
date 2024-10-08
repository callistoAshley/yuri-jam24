#include "layer.h"
#include "sensible_nums.h"
#include "utility/vec.h"
#include <assert.h>

typedef struct
{
    void *entry;
    LayerEntry next;
} LayerEntryData;

#define INITIAL_BUFFER_CAP 32
#define INITIAL_BUFFER_SIZE sizeof(TransformEntryData) * INITIAL_BUFFER_CAP

void layer_init(Layer *layer, thing_draw_fn draw)
{
    vec_init_with_capacity(&layer->entries, sizeof(LayerEntryData),
                           INITIAL_BUFFER_CAP);
    layer->next = 0;

    layer->draw = draw;
}

void layer_free(Layer *layer) { vec_free(&layer->entries); }

LayerEntry layer_add(Layer *layer, void *thing)
{
    LayerEntry key = layer->next;

    if (layer->next == layer->entries.len)
    {
        LayerEntryData entry = {
            .entry = thing,
        };

        assert((usize)entry.entry != LAYER_ENTRY_FREE);

        vec_push(&layer->entries, &entry);
        layer->next++;
    }
    else
    {
        LayerEntryData *entry = vec_get(&layer->entries, layer->next);
        assert(entry != NULL);
        assert((usize)entry->entry == LAYER_ENTRY_FREE);

        layer->next = entry->next;

        entry->entry = thing;
    }

    return key;
}

void layer_remove(Layer *layer, LayerEntry entry)
{
    LayerEntryData *data = vec_get(&layer->entries, entry);
    assert(data != NULL);
    assert((usize)data->entry != LAYER_ENTRY_FREE);

    data->entry = (void *)LAYER_ENTRY_FREE;
    data->next = layer->next;
    layer->next = entry;
}

void layer_draw(Layer *layer, void *context, WGPURenderPassEncoder pass)
{
    for (usize i = 0; i < layer->entries.len; i++)
    {
        LayerEntryData *data = vec_get(&layer->entries, i);
        if ((usize)data->entry == LAYER_ENTRY_FREE)
            continue;
        if (layer->draw)
            layer->draw(data->entry, context, pass);
    }
}
