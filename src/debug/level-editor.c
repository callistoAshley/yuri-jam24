#include "level-editor.h"
#include "windows/brush.h"
#include "windows/new-map.h"
#include "windows/tilemap-editor.h"

static void new_map_callback(void *wnd_cont, NewMapInfo info)
{
    LevelEditor *editor = ((WindowContainer *)wnd_cont)->owner;

    wnd_brush_init_tilemap(editor->brush_wnd, info.input_width, info.input_height);
}

LevelEditor *lvledit_init(Graphics *graphics)
{
    LevelEditor *editor = calloc(1, sizeof(LevelEditor));
    PTR_ERRCHK(editor, "lvledit_init: calloc failure.");

    editor->container = wndcont_init(editor, graphics);
    editor->brush_wnd = wndcont_add(editor->container, brush_window);

    return editor;
}

void lvledit_update(LevelEditor *editor)
{
    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true))
        {
            if (igMenuItem_Bool("New", NULL, false, true))
            {
                Window *wnd = wndcont_add(editor->container, new_map_window);
                wnd_new_map_set_done_callback(wnd, new_map_callback);
            }
            if (igMenuItem_Bool("Load", NULL, false, true))
            {
            }
            if (igMenuItem_Bool("Save", NULL, false, true))
            {
            }
            if (igMenuItem_Bool("Exit", NULL, false, true))
            {
                editor->request_quit = true;
            }
            igEndMenu();
        }
        igEndMainMenuBar();
    }
    wndcont_update(editor->container);
}

void lvledit_free(LevelEditor *editor)
{
    wndcont_free(editor->container);
    free(editor);
}
