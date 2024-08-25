#include "tex_manager.h"
#include "sensible_nums.h"
#include "utility/graphics.h"
#include "utility/macros.h"
#include "webgpu.h"
#include <SDL3_image/SDL_image.h>

// we don't need a union here, because we already have something that keeps
// track of if an entry is free: the reference count!

void texture_manager_init(TextureManager *manager)
{
    vec_init(&manager->texture_views, sizeof(WGPUTextureView));
    vec_init(&manager->textures, sizeof(WGPUTexture));
    vec_init(&manager->entries, sizeof(TextureEntry *));

    manager->next = 0;
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
        .index = manager->next,
        .path = new_path,
    };

    if (manager->next == manager->entries.len)
    {
        TextureEntry *new_entry = malloc(sizeof(TextureEntry));
        *new_entry = entry;

        vec_push(&manager->entries, &new_entry);
        vec_push(&manager->textures, &texture);
        vec_push(&manager->texture_views, &view);
        manager->next++;

        return new_entry;
    }
    else
    {
        TextureEntry *free_entry =
            (TextureEntry *)vec_get(&manager->entries, manager->next);
        WGPUTexture *free_texture = vec_get(&manager->textures, manager->next);
        WGPUTextureView *free_view =
            vec_get(&manager->texture_views, manager->next);

        assert(free_entry != NULL);
        assert(free_texture != NULL);
        assert(free_view != NULL);

        assert(free_entry->ref_count == 0);
        manager->next = free_entry->index;

        *free_entry = entry;
        *free_texture = texture;
        *free_view = view;

        return free_entry;
    }
}

void texture_manager_unload(TextureManager *manager, TextureEntry *entry)
{
    assert(entry->ref_count > 0);
    entry->ref_count--;

    if (entry->ref_count == 0)
    {
        free((void *)entry->path);

        WGPUTexture *texture = vec_get(&manager->textures, entry->index);
        WGPUTextureView *view = vec_get(&manager->texture_views, entry->index);

        assert(texture != NULL);
        assert(view != NULL);

        wgpuTextureViewRelease(*view);
        wgpuTextureRelease(*texture);

        u32 temp = manager->next;
        manager->next = entry->index;
        entry->index = temp;
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
