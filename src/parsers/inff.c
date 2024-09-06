#include "inff.h"
#include "utility/files.h"

INFF *inff_parse(char *path, char out_err_msg[256])
{
    INFF *inff = calloc(1, sizeof(INFF));
    read_entire_file(path, (char **)&inff->raw_buffer, &inff->raw_buffer_len);

    u64 offset = 0;
    memcpy(inff->magic, inff->raw_buffer, 4);
    offset += 4;

    if (strncmp(inff->magic, "INFF", 4))
    {
        snprintf(out_err_msg, 256, "INFF magic number validation failure.");
        return NULL;
    }
    memcpy(&inff->num_chunks, inff->raw_buffer + offset, 4);
    offset += 4;
    inff->chunks = calloc(inff->num_chunks, sizeof(INFFChunk));

    for (uint32_t i = 0; i < inff->num_chunks; i++)
    {
        INFFChunk *chunk = inff->chunks + i;
        memcpy(chunk->id, inff->raw_buffer + offset, 4);
        offset += 4;
        memcpy(&chunk->data_len, inff->raw_buffer + offset, 4);
        offset += 4;
        chunk->data = inff->raw_buffer + offset;
        offset += chunk->data_len;
    }

    return inff;
}

INFFChunk *inff_get_chunk(INFF *inff, char id[4])
{
    for (INFFChunk *chunk = inff->chunks;
         chunk != inff->chunks + inff->num_chunks; chunk++)
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
    free(inff->chunks);
    free(inff->raw_buffer);
    free(inff);
}
