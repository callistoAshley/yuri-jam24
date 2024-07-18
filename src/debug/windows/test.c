#include "test.h"

typedef struct
{

} TestWndState;

Window test_window =
{
    .init_fn = wnd_test_init,
    .update_fn = wnd_test_update,
    .free_fn = wnd_test_free,
};

void wnd_test_init(Window *self)
{
    self->userdata = calloc(1, sizeof(TestWndState));
}

void wnd_test_update(Window *self)
{
    (void)self;
}

void wnd_test_free(Window *self)
{
    free(self->userdata);
}
