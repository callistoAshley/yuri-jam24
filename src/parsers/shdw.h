#pragma once

#include <cglm/struct.h>
#include "sensible_nums.h"

typedef struct
{
    u32 point_count;
    vec2s *points;
} Cell;

typedef struct
{
    char magic[4];
    u32 cell_count, cell_width, cell_height;
    Cell *cells;
} SHDWFile;

SHDWFile *shdw_parse(const char *path, char out_err_msg[256]);
void shdw_free(SHDWFile *shdw);
