#pragma once

#include <wgpu.h>
#include "graphics/wgpu_resources.h"
#include "core_types.h"
#include "sensible_nums.h"
#include "utility/vec.h"

// reference counted!
typedef struct
{
    u32 ref_count;
    u32 index;
    const char *path; // DO NOT MODIFY!
} TextureEntry;

typedef struct
{
    vec texture_views; // vec<WGPUTextureView>
    vec textures;      // vec<WGPUTexture>
    vec entries;       // a list of TextureEntry
} TextureManager;

void texture_manager_init(TextureManager *manager);
void texture_manager_free(TextureManager *manager);

// returns a reference to the texture at the given path
// O(n) time complexity. Will increment the reference count of the texture if it
// already exists
// if the texture does not exist, it will be loaded
// NOTE: the path IS copied!
TextureEntry *texture_manager_load(TextureManager *manager, const char *path,
                                   WGPUResources *resources);
// will decrement the reference count of the texture, and unload it if it
// reaches 0
void texture_manager_unload(TextureManager *manager, TextureEntry *entry);

TextureEntry *texture_manager_register(TextureManager *manager,
                                       WGPUTexture texture, const char *path);

WGPUTexture texture_manager_get_texture(TextureManager *manager,
                                        TextureEntry *entry);
WGPUTextureView texture_manager_get_texture_view(TextureManager *manager,
                                                 TextureEntry *entry);
