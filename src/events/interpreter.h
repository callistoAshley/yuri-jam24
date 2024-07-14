#pragma once

typedef struct
{
    int cond_register;
    char choice_buffer[256];
} Interpreter;

Interpreter *interpreter_init(char **files, int num_files);
