#include "lvl-parser.h"

LvlFile *lvl_parse(char *file_path, char out_err_msg[256])
{
    LvlFile *lvl;
    FILE *file;
    char buffer[256];

    lvl = calloc(1, sizeof(LvlFile));
    PTR_ERRCHK(lvl, "lvl_parse: calloc failure.");

    vec_init(lvl->chunks, sizeof(LvlFileChunk));

    file = fopen(file_path, "r");
    if (!file)
    {
        free(lvl);
        snprintf(out_err_msg, 256, "lvl_parse: %s: no such file or directory.", file_path);
        return NULL;
    }

    fread(buffer, 1, 3, file);
    if (strncmp(buffer, "LVL", 3))
    {
        vec_free(lvl->chunks);
        free(lvl);
        snprintf(out_err_msg, 256, "lvl_parse: %s: magic number validation failure.", file_path);
        return NULL;
    }

    fread(&lvl->num_chunks, 4, 1, file);
    
    for (int i = 0; i < lvl->num_chunks; i++)
    {
        LvlFileChunk *chunk = calloc(1, sizeof(LvlFileChunk));
        PTR_ERRCHK(chunk, "lvl_parse: calloc failure.");

        fread(chunk->tag, 1, 4, file);
        fread(&chunk->data_len, 1, sizeof(chunk->data_len), file);
        chunk->data = malloc(chunk->data_len);
        PTR_ERRCHK(chunk->data, "lvl_parse: malloc failure.");
        fread(chunk->data, 1, chunk->data_len, file);
    }

    return lvl;
}
