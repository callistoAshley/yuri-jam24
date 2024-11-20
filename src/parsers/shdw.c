#include "shdw.h"
#include "utility/files.h"
#include <string.h>

SHDWFile *shdw_parse(const char *path, char out_err_msg[256])
{
    SHDWFile *shdw = calloc(1, sizeof(SHDWFile));

    char *buffer;
    isize buffer_len;
    read_entire_file(path, &buffer, &buffer_len);

    if (!buffer)
    {
        snprintf(out_err_msg, 256, "Failed to read file: %s", path);
        return NULL;
    }

    u64 offset = 0;
    memcpy(shdw->magic, buffer, 4);
    offset += 4;

    if (strncmp(shdw->magic, "SHDW", 4))
    {
        snprintf(out_err_msg, 256, "SHDW magic number validation failure.");
        return NULL;
    }

    memcpy(&shdw->cell_count, buffer + offset, 4);
    offset += 4;
    memcpy(&shdw->cell_width, buffer + offset, 4);
    offset += 4;
    memcpy(&shdw->cell_height, buffer + offset, 4);
    offset += 4;

    shdw->cells = calloc(shdw->cell_count, sizeof(Cell));
    for (u32 i = 0; i < shdw->cell_count; i++)
    {
        Cell *cell = shdw->cells + i;
        memcpy(&cell->line_count, buffer + offset, 4);
        offset += 4;

        cell->lines = calloc(cell->line_count, sizeof(Line));
        memcpy(cell->lines, buffer + offset, cell->line_count * sizeof(Line));
        offset += cell->line_count * sizeof(Line);
    }

    free(buffer);
    return shdw;
}

void shdw_free(SHDWFile *shdw)
{
    for (u32 i = 0; i < shdw->cell_count; i++)
    {
        free(shdw->cells[i].lines);
    }
    free(shdw->cells);
    free(shdw);
}
