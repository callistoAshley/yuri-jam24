#include "window-container.h"

static void window_free_fn(usize sz, void *elem)
{
    (void)sz;

    Window *window = elem;
    puts("calling free_fn");
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

    vec_init(&cont->windows, sizeof(Window));

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

    vec_push(&cont->windows, result);

    return result;
}

void wndcont_remove(WindowContainer *cont, Window *window)
{
    usize index;
    bool found = false;

    for (usize i = 0; i < cont->windows.len; i++)
    {
        if (vec_get(&cont->windows, i) == window)
        {
            index = i;
            found = true;
            break;
        }
    }

    if (!found) return;

    puts("calling vec_remove");
    vec_remove(&cont->windows, index, NULL);
    puts("calling window_free_fn");
    window_free_fn(0, window);
}

void wndcont_update(WindowContainer *cont)
{
    for (usize i = 0; i < cont->windows.len; i++)
    {
        Window *window = vec_get(&cont->windows, i);
        window->update_fn(window);
        if (window->children) wndcont_update(window->children);

        if (window->remove)
        {
            printf("removing %p\n", (void *)window);
            wndcont_remove(cont, window);
            i--;
        }
    }
}

void wndcont_free(WindowContainer *cont)
{
    vec_free_with(&cont->windows, window_free_fn);
    free(cont);
}
