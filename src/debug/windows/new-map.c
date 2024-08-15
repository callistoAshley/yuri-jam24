#include "new-map.h"

typedef struct
{
    char input_name[256];
    int input_width, input_height;
} NewMapWndState;

Window new_map_window =
{
    .init_fn = wnd_new_map_init,
    .update_fn = wnd_new_map_update,
    .free_fn = wnd_new_map_free,
};

void wnd_new_map_init(Window *self)
{
    self->userdata = calloc(1, sizeof(NewMapWndState));
}

void wnd_new_map_update(Window *self)
{
    NewMapWndState *state = self->userdata;
    static const ImVec2 wnd_size = {200, 150};

    igSetNextWindowSize(wnd_size, ImGuiCond_Once);
    if (igBegin("New Map", NULL, 0))
    {
        igInputText(
            "Name", 
            state->input_name, 
            sizeof(state->input_name), 
            ImGuiInputTextFlags_None,
            NULL,
            NULL
        );

        igInputInt(
            "Width",
            &state->input_width,
            1,
            0,
            ImGuiInputTextFlags_None
        );

        igInputInt(
            "Height",
            &state->input_height,
            1,
            0,
            ImGuiInputTextFlags_None
        );

        if (igSmallButton("OK"))
        {
            // no spaces and no empty strings
            if (strspn(state->input_name, " ") == strlen(state->input_name))
            {
                puts("bad map name");
            }
            else
            {
                self->remove = true;
            }
        }

        if (state->input_width < 0) state->input_width = 0;
        if (state->input_height < 0) state->input_height = 0;
    }
    igEnd();
}

void wnd_new_map_free(Window *self)
{
    free(self->userdata);
}
