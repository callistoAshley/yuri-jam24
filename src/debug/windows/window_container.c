#include "window_container.h"

static void free_window(Window *window)
{
    window->free_fn(window);
    wndcont_free(window->children);
    free(window);
}

WindowContainer *wndcont_init(void *owner, Graphics *graphics)
{
    WindowContainer *cont = calloc(1, sizeof(WindowContainer));
    PTR_ERRCHK(cont, "wndcont_init: calloc failure.");

    cont->owner = owner;
    cont->graphics = graphics;
    cont->windows = linked_list_init();

    return cont;
}

Window *wndcont_add(WindowContainer *cont, Window window)
{
    Window *result = malloc(sizeof(Window));
    PTR_ERRCHK(result, "wndcont_add: malloc failure.");
    memcpy(result, &window, sizeof(Window));

    result->children = wndcont_init(cont->owner, cont->graphics);
    result->wnd_cont = cont;
    result->init_fn(result);

    linked_list_append(cont->windows, result);

    return result;
}

void wndcont_remove(WindowContainer *cont, Window *window)
{
    for (LinkedListNode *node = cont->windows->first; node; node = node->next)
    {
        if (node->data == window)
        {
            free_window(window);
            linked_list_remove(cont->windows, linked_list_index_of(cont->windows, window));
            return;            
        }
    }
}

void wndcont_update(WindowContainer *cont)
{
    for (LinkedListNode *node = cont->windows->first; node; node = node->next)
    {
        Window *window = node->data;
        window->update_fn(window);
        if (window->children) wndcont_update(window->children);

        if (window->remove)
        {
            wndcont_remove(cont, window);
            node = cont->windows->first; // hack: just begin the iteration again lol
        }
    }
}

void wndcont_free(WindowContainer *cont)
{
    for (LinkedListNode *node = cont->windows->first; node; node = node->next)
    {
        free_window(node->data);
    }
    linked_list_free(cont->windows);
    free(cont);
}
