#include "tilemap-editor.h"

typedef struct
{
    uint32_t width, height;
    uint16_t *tilemap;
} TmapEditWndState;

Window tmap_edit_window = {
    .id = "tmap_edit", 

    .init_fn = wnd_tmap_edit_init,
    .update_fn = wnd_tmap_edit_update,
    .free_fn = wnd_tmap_edit_free,
};

void wnd_tmap_edit_init(Window *self)
{
    self->userdata = calloc(1, sizeof(TmapEditWndState));
}

void wnd_tmap_edit_init_tilemap(Window *self, uint32_t width, uint32_t height)
{
    TmapEditWndState *state = self->userdata;
    state->width = width;
    state->height = height;
    state->tilemap = calloc(width * height, sizeof(uint32_t));
}

void wnd_tmap_edit_update(Window *self)
{
    TmapEditWndState *state = self->userdata;
    static const ImVec2 wnd_size = {320, 240};

    igSetNextWindowSize(wnd_size, ImGuiCond_Once);
    if (igBegin("Tilemap Editor", NULL, 0))
    {
        if (!state->tilemap)
        {
            igText("The tilemap has not been initialized.");
        }
        else
        {
        }
    }
    igEnd();
}

void wnd_tmap_edit_free(Window *self) { free(self->userdata); }
