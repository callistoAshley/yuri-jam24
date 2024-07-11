#include "command-funcs.h"

struct
{
    char *name;
    CommandFn func;
} commands[] = {
    {"text", command_text},
    {"test_command", command_test},
    {NULL, NULL} // sentinel
}
