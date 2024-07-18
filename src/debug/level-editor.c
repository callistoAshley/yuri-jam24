#include "level-editor.h"
#include "windows/test.h"

LevelEditor *lvledit_init(void)
{
    LevelEditor *editor = calloc(1, sizeof(LevelEditor));
    PTR_ERRCHK(editor, "lvledit_init: calloc failure.");

    editor->container = wndcont_init();
    wndcont_add(editor->container, test_window);

    return editor;
}

void lvledit_update(LevelEditor *editor)
{
    wndcont_update(editor->container);
}

void lvledit_free(LevelEditor *editor)
{
    wndcont_free(editor->container);
    free(editor);
}
