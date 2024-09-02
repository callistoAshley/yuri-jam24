#include "tilemap-editor.h"
#include "window-container.h"
#include "graphics/graphics.h"

typedef struct
{
    uint32_t width, height;
    Tilemap tilemap;

    TextureEntry *tileset_tex;
    WGPUTextureView tileset_tex_non_srgb;

    Transform transform;

    u32 *tile_data;
} TmapEditWndState;

Window tmap_edit_window = 
{
    .id = "tmap_edit", 

    .init_fn = wnd_tmap_edit_init,
    .update_fn = wnd_tmap_edit_update,
    .free_fn = wnd_tmap_edit_free,
};

void wnd_tmap_edit_init(Window *self)
{
    self->userdata = calloc(1, sizeof(TmapEditWndState));
}

void wnd_tmap_edit_init_tilemap(
    Window *self, 
    uint32_t width, 
    uint32_t height,
    TextureEntry *tileset_tex,
    WGPUTextureView tileset_tex_non_srgb)
{
    TmapEditWndState *state = self->userdata;
    state->width = width;
    state->height = height;
    state->tileset_tex = tileset_tex;
    state->tileset_tex_non_srgb = tileset_tex_non_srgb;
    state->transform = transform_from_xyz(0, 0, 0);
    state->tile_data = calloc(state->width * state->height, sizeof(u32));

    tilemap_init(
        &state->tilemap, 
        self->wnd_cont->graphics,
        tileset_tex, 
        transform_manager_add(&self->wnd_cont->graphics->transform_manager, state->transform),
        state->width,
        state->height,
        8,
        state->tile_data
    );
}

void wnd_tmap_edit_update(Window *self)
{
    TmapEditWndState *state = self->userdata;
    static const ImVec2 wnd_size = {320, 240};

    igSetNextWindowSize(wnd_size, ImGuiCond_Once);
    if (igBegin("Tilemap Editor", NULL, 0))
    {
        if (!state->tile_data)
        {
            igText("The tilemap has not been initialized.");
        }
        else
        {
            ImDrawList *draw_list = igGetWindowDrawList();
            // render the entire tilemap into one WGPUTextureView?
            // take a look at the ImDrawList API
        }
    }
    igEnd();
}

void wnd_tmap_edit_free(Window *self) 
{ 
    TmapEditWndState *state = self->userdata;

    tilemap_free(&state->tilemap, self->wnd_cont->graphics);
    free(state->tile_data);
    free(state); 
}
