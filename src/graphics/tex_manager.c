#include "tex_manager.h"
#include "sensible_nums.h"
#include "utility/graphics.h"
#include "utility/macros.h"
#include "webgpu.h"
#include <SDL3_image/SDL_image.h>

#include <assert.h>

// we don't need a union here, because we already have something that keeps
// track of if an entry is free: the reference count!

void texture_manager_init(TextureManager *manager)
{
    vec_init(&manager->texture_views, sizeof(WGPUTextureView));
    vec_init(&manager->textures, sizeof(WGPUTexture));
    vec_init(&manager->entries, sizeof(TextureEntry *));
}

static void free_texture_view(usize index, void *data)
{
    (void)index;
    WGPUTextureView *view = data;
    wgpuTextureViewRelease(*view);
}
static void free_texture(usize index, void *data)
{
    (void)index;
    WGPUTexture *texture = data;
    wgpuTextureRelease(*texture);
}
static void free_entry(usize index, void *data)
{
    (void)index;
    TextureEntry *entry = *(TextureEntry **)data;
    if (entry->ref_count > 0)
        free((void *)entry->path);
    free(entry);
}
void texture_manager_free(TextureManager *manager)
{
    vec_free_with(&manager->texture_views, free_texture_view);
    vec_free_with(&manager->textures, free_texture);
    vec_free_with(&manager->entries, free_entry);
}

TextureEntry *texture_manager_load(TextureManager *manager, const char *path,
                                   WGPUResources *resources)
{
    // check if the texture is already loaded
    for (usize i = 0; i < manager->entries.len; i++)
    {
        TextureEntry *entry = *(TextureEntry **)vec_get(&manager->entries, i);
        if (entry->ref_count == 0)
            continue;
        if (strcmp(entry->path, path) == 0)
        {
            entry->ref_count++;
            return entry;
        }
    }

    // load texture
    SDL_Surface *surface = IMG_Load(path);
    SDL_PTR_ERRCHK(surface, "failed to load image");
    WGPUTexture texture = texture_from_surface(
        surface, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        resources);
    SDL_DestroySurface(surface);

    return texture_manager_register(manager, texture, path);
}

TextureEntry *texture_manager_register(TextureManager *manager,
                                       WGPUTexture texture, const char *path)
{
    WGPUTextureView view = wgpuTextureCreateView(texture, NULL);

    const char *new_path = strdup(path);
    TextureEntry entry = {
        .ref_count = 1,
        .index = manager->entries.len,
        .path = new_path,
    };

    TextureEntry *new_entry = malloc(sizeof(TextureEntry));
    *new_entry = entry;

    vec_push(&manager->entries, &new_entry);
    vec_push(&manager->textures, &texture);
    vec_push(&manager->texture_views, &view);

    return new_entry;
}

void texture_manager_unload(TextureManager *manager, TextureEntry *entry)
{
    assert(entry->ref_count > 0);
    entry->ref_count--;

    if (entry->ref_count == 0)
    {
        WGPUTexture texture;
        WGPUTextureView view;

        // swap remove avoids shifting elements around by swapping in the last
        // element and decrementing the length of the array
        // this is important to
        // 1) keep the arrays contiguous
        // 2) make sure everything has the correct index
        vec_swap_remove(&manager->textures, entry->index, &texture);
        vec_swap_remove(&manager->texture_views, entry->index, &view);
        vec_swap_remove(&manager->entries, entry->index, NULL);

        // if this happens to be the last element, we don't need to update
        // anything!
        if (entry->index < manager->entries.len)
        {
            TextureEntry *moved_entry =
                *(TextureEntry **)vec_get(&manager->entries, entry->index);
            // update the moved entry's index to where it is now
            moved_entry->index = entry->index;
        }

        wgpuTextureRelease(texture);
        wgpuTextureViewRelease(view);
        free((void *)entry->path);

        free(entry);
    }
}

WGPUTexture texture_manager_get_texture(TextureManager *manager,
                                        TextureEntry *entry)
{
    WGPUTexture *texture = vec_get(&manager->textures, entry->index);
    assert(texture != NULL);
    return *texture;
}

WGPUTextureView texture_manager_get_texture_view(TextureManager *manager,
                                                 TextureEntry *entry)
{
    WGPUTextureView *view = vec_get(&manager->texture_views, entry->index);
    assert(view != NULL);
    return *view;
}
