#include "brush.h"
#include "tilemap-editor.h"
#include "window-container.h"

typedef struct
{
    bool solid;

    TextureEntry *tileset_tex;
    WGPUTextureView non_srgb_view;
} BrushWndState;

Window brush_window = {
    .id = "brush",

    .init_fn = wnd_brush_init,
    .update_fn = wnd_brush_update,
    .free_fn = wnd_brush_free,
};

void wnd_brush_init(Window *self)
{
    self->userdata = calloc(1, sizeof(BrushWndState));

    wndcont_add(self->children, tmap_edit_window);

    wnd_brush_set_tileset(self, "./assets/textures/tileset_test.png");
}

void wnd_brush_set_tileset(Window *self, char *tileset)
{
    BrushWndState *state = self->userdata;
    WindowContainer *wnd_cont = self->wnd_cont;
    Graphics *graphics = wnd_cont->graphics;

    if (state->tileset_tex)
    {
        texture_manager_unload(&graphics->texture_manager, state->tileset_tex);
        wgpuTextureViewRelease(state->non_srgb_view);
    }

    state->tileset_tex = texture_manager_load(&graphics->texture_manager,
                                              tileset, &graphics->wgpu);
    WGPUTexture tileset_texture = texture_manager_get_texture(
        &graphics->texture_manager, state->tileset_tex);
    WGPUTextureViewDescriptor view_desc = {
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = 1,
        .arrayLayerCount = 1,
    };
    state->non_srgb_view = wgpuTextureCreateView(tileset_texture, &view_desc);
}

void wnd_brush_update(Window *self)
{
    BrushWndState *state = self->userdata;

    static const ImVec2 wnd_size = {320, 100};

    igSetNextWindowSize(wnd_size, ImGuiCond_Once);
    if (igBegin("Brush", NULL, 0))
    {
        ImVec2 tset_view_size = {96, 16};
        static const ImVec2 uv0 = {0.0, 0.0}, uv1 = {1.0, 1.0};
        static const ImVec4 tint_col = {1, 1, 1, 1}, border_col = {0, 0, 0, 0};

        igCheckbox("Solid", &state->solid);
        igSeparatorText("Tileset");
        igImage(state->non_srgb_view, tset_view_size, uv0, uv1, tint_col,
                border_col);
    }
    igEnd();
}

void wnd_brush_free(Window *self)
{
    BrushWndState *state = self->userdata;
    WindowContainer *wnd_cont = self->wnd_cont;

    texture_manager_unload(&wnd_cont->graphics->texture_manager,
                           state->tileset_tex);
    wgpuTextureViewRelease(state->non_srgb_view);

    free(self->userdata);
}
