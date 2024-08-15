#include "level-editor.h"
#include "windows/tilemap-editor.h"

LevelEditor *lvledit_init(void)
{
    LevelEditor *editor = calloc(1, sizeof(LevelEditor));
    PTR_ERRCHK(editor, "lvledit_init: calloc failure.");

    editor->container = wndcont_init();
    wndcont_add(editor->container, tmap_edit_window);

    return editor;
}

void lvledit_update(LevelEditor *editor)
{
    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true))
        {
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
