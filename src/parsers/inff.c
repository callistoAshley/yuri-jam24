#include "inff.h"

INFF *inff_parse(char *path, char out_err_msg[256])
{
    INFF *inff = calloc(1, sizeof(INFF));
    FILE *file = fopen(path, "r");

    fread(inff->magic, 1, 4, file);
    if (strncmp(inff->magic, "INFF", 4))
    {
        snprintf(out_err_msg, 256, "INFF magic number validation failure.");
        return NULL;
    }
    fread(&inff->num_chunks, sizeof(inff->num_chunks), 1, file);
    inff->chunks = calloc(inff->num_chunks, sizeof(INFFChunk));

    for (uint32_t i = 0; i < inff->num_chunks; i++)
    {
        INFFChunk *chunk = inff->chunks + i;
        fread(chunk->id, 1, 4, file);
        fread(&chunk->data_len, 1, sizeof(chunk->data_len), file);
        chunk->data = malloc(chunk->data_len);
        fread(chunk->data, chunk->data_len, 1, file);
    }

    fclose(file);
    return inff;
}

INFFChunk *inff_get_chunk(INFF *inff, char id[4])
{
    for (INFFChunk *chunk = inff->chunks; chunk != inff->chunks + inff->num_chunks; chunk++)
    {
        if (!strncmp(chunk->id, id, 4))
        {
            return chunk;
        }
    }
    return NULL;
}

void inff_free(INFF *inff)
{
    for (INFFChunk *chunk = inff->chunks; chunk != inff->chunks + inff->num_chunks; chunk++)
    {
        free(chunk->data);
    }
    free(inff->chunks);
    free(inff);
}
