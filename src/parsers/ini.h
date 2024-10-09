#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "utility/files.h"
#include "utility/linked_list.h"
#include "utility/macros.h"

typedef struct
{
    char *key, *value;
} IniPair;

typedef struct
{
    char *name;
    LinkedList *pairs;
} IniSection;

typedef struct
{
    LinkedList *sections;
} Ini;

Ini *ini_parse_string_n(const char *string, size_t string_len,
                        char out_err_msg[256]);
Ini *ini_parse_string(const char *string, char out_err_msg[256]);
Ini *ini_parse_file(const char *path, char out_err_msg[256]);
void ini_free(Ini *ini);
