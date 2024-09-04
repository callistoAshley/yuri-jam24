#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utility/linked_list.h"

typedef struct
{
    char id[4];
    uint32_t data_len;
    unsigned char *data;
} INFFChunk;

typedef struct
{
    char magic[4];
    uint32_t num_chunks;    
    INFFChunk *chunks;
} INFF;

INFF *inff_parse(char *path, char out_err_msg[256]);
INFFChunk *inff_get_chunk(INFF *inff, char id[4]);
void inff_free(INFF *inff);
