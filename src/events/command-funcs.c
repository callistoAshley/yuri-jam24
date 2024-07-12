#include "command-funcs.h"

int command_text(Command *command)
{
    (void)command;
    printf("command_text\n");
    return 1;
}

int command_test(Command *command)
{
    (void)command;
    printf("command_test\n");
    return 1;
}
