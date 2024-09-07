#pragma once

#include "quad_manager.h"
#include "tex_manager.h"
#include "transform_manager.h"

typedef struct
{
    QuadEntry entry;
    TransformEntry transform_entry;
    u32 texture_id;
} SpriteData;

typedef struct
{
    vec data; // SpriteData
    WGPUBuffer buffer;

    // the transform and quad managers work by storing an index to the next free
    // entry inside every free entry. they're able to directly upload the
    // quad/transform vec to the GPU by using unions, and expecting no code to
    // read/write to dead keys.

    // we CANNOT use the same trick as the quad and transform managers, because
    // we have to keep the instance buffer contiguous.
    // when drawing using an instance buffer, the GPU will walk through the
    // buffer in order. it's not possible to have any gaps without using
    // multiDrawIndirect (which is more complicated)

    // this means we have to have a different data structure for the sprite
    // layer.

    // for convenience it's really important to hand out "keys" when adding
    // things to the layer. however, these can't be directly used to index into
    // the data array, because we have to keep the data array contiguous. so we
    // need a way to map these keys to indices. unfortunately, this means we
    // have to use some kind of hashmap, as *well* as a free list. (otherwise we
    // might starve ourselves of keys)

    // so. when adding things to the layer, first we take the first free key.
    // (if none are available, we hand out a data.len)

    // we then append the data to the end of the data array, and store the index
    // in the hashmap. the hashmap is a key -> index mapping, so whenever we
    // need to access an index we have to look it up in the hashmap first.

    // when removing things from the layer, we perform a "swap remove"
    // operation- we swap out the last element with the element we want to
    // remove, and then remove the last element. this way we don't have to shift
    // everything down.
    // however, we also have to update the hashmap to reflect this change. we
    // have to update the index of the last element to reflect its new position.
    // afterwards we can mark that key as free by adding it to the free list
} SpriteLayer;
