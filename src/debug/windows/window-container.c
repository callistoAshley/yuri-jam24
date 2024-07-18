#include "window-container.h"

static void window_free_fn(usize sz, void *elem)
{
    Window *window = elem;
    (void)sz;
    window->free_fn(window);
    free(window);
}

WindowContainer *wndcont_init(void)
{
    WindowContainer *cont = calloc(1, sizeof(WindowContainer));
    PTR_ERRCHK(cont, "wndcont_init: calloc failure.");

    vec_init(&cont->windows, sizeof(Window));

    return cont;
}

Window *wndcont_add(WindowContainer *cont, Window window)
{
    Window *result = malloc(sizeof(Window));
    PTR_ERRCHK(result, "wndcont_add: malloc failure.");

    memcpy(result, &window, sizeof(Window));
    result->init_fn(result);

    vec_push(&cont->windows, result);

    return result;
}

void wndcont_remove(WindowContainer *cont, Window *window)
{
    usize index;

    for (usize i = 0; i < cont->windows.len; i++)
    {
        if (vec_get(&cont->windows, i) == window)
        {
            index = i;
            break;
        }
    }
    vec_remove(&cont->windows, index, NULL);

    window->free_fn(window);
}

void wndcont_update(WindowContainer *cont)
{
    for (usize i = 0; i < cont->windows.len; i++)
    {
        Window *window = vec_get(&cont->windows, i);
        window->update_fn(window);
    }
}

void wndcont_free(WindowContainer *cont)
{
    vec_free_with(&cont->windows, window_free_fn);
    free(cont);
}
