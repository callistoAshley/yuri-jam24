#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility/macros.h"
#include "utility/vec.h"

typedef struct
{
    char tag[4];
    uint32_t data_len;
    uint8_t *data;
} LvlFileChunk;

typedef struct
{
    int num_chunks;
    vec *chunks;
} LvlFile;

LvlFile *lvl_parse(char *file_path, char out_err_msg[256]);
