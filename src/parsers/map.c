#include "map.h"
#include "box2d/math_functions.h"
#include "parsers/inff.h"
#include "utility/log.h"
#include "utility/macros.h"

typedef struct
{
    u32 offset;
    u8 *data;
} ChunkDataReader;

// FIXME: none of these do ANY bounds checks!
static u32 chunk_read_u32(ChunkDataReader *reader)
{
    // FIXME: this performs an unaligned read
    // Not sure if that counts as undefined behavior
    u32 value = *(u32 *)(reader->data + reader->offset);
    reader->offset += 4;
    return value;
}

static i32 chunk_read_i32(ChunkDataReader *reader)
{
    i32 value = *(i32 *)(reader->data + reader->offset);
    reader->offset += 4;
    return value;
}

static f32 chunk_read_f32(ChunkDataReader *reader)
{
    f32 value = *(f32 *)(reader->data + reader->offset);
    reader->offset += 4;
    return value;
}

// NOTE: this function allocates memory, and needs to be freed!
static char *chunk_read_string(ChunkDataReader *reader)
{
    u32 len = chunk_read_u32(reader);
    char *str = malloc(len + 1);
    memcpy(str, reader->data + reader->offset, len);
    str[len] = '\0';
    reader->offset += len;
    return str;
}

// ids are char[4] and NOT null-terminated so we can't use strcmp
#define IDEQ(a, b) strncmp(a, b, 4) == 0
#define IDNEQ(a, b) strncmp(a, b, 4) != 0

static void parse_map_layers(INFF *inff, Map *map, MapLayer *into,
                             u32 *current_layer, u32 *chunk_index)
{
    INFFChunk chunk = inff->chunks[*chunk_index];
    (*chunk_index)++;
    ChunkDataReader reader = {0, chunk.data};

    // FIXME this should be handled like INFO
    if (IDEQ(chunk.id, "TSET"))
    {
        map->tileset.name = chunk_read_string(&reader);
        map->tileset.image_path = chunk_read_string(&reader);
    }
    else if (IDEQ(chunk.id, "TINF"))
    {
        MapLayer layer;
        layer.name = chunk_read_string(&reader);
        layer.class_name = chunk_read_string(&reader);
        layer.data.tile.parallax_x = chunk_read_f32(&reader);
        layer.data.tile.parallax_y = chunk_read_f32(&reader);
        layer.type = Layer_Tile;

        chunk = inff->chunks[*chunk_index];
        (*chunk_index)++;
        if (IDNEQ(chunk.id, "TDAT"))
            FATAL("Expected TDAT chunk after TINF chunk");
        reader.offset = 0;
        reader.data = chunk.data;

        layer.data.tile.tiles = malloc(sizeof(i32) * map->width * map->height);
        for (u32 i = 0; i < map->width * map->height; i++)
            layer.data.tile.tiles[i] = chunk_read_i32(&reader);

        into[*current_layer] = layer;
        (*current_layer)++;
    }
    else if (IDEQ(chunk.id, "OINF"))
    {
        MapLayer layer;
        layer.name = chunk_read_string(&reader);
        layer.class_name = chunk_read_string(&reader);
        layer.data.object.object_len = chunk_read_u32(&reader);
        layer.type = Layer_Object;

        chunk = inff->chunks[*chunk_index];
        (*chunk_index)++;
        if (IDNEQ(chunk.id, "ODAT"))
            FATAL("Expected ODAT chunk after OINF chunk");
        reader.offset = 0;
        reader.data = chunk.data;

        layer.data.object.objects =
            malloc(sizeof(Object) * layer.data.object.object_len);
        for (u32 i = 0; i < layer.data.object.object_len; i++)
        {
            Object *object = &layer.data.object.objects[i];
            object->name = chunk_read_string(&reader);
            object->class_name = chunk_read_string(&reader);
            object->x = chunk_read_f32(&reader);
            object->y = chunk_read_f32(&reader);
            object->width = chunk_read_f32(&reader);
            object->height = chunk_read_f32(&reader);

            // TODO validate this
            object->type = chunk_read_u32(&reader);

            object->polygon_len = chunk_read_u32(&reader);
            object->polygons = malloc(sizeof(b2Vec2) * object->polygon_len);
            for (u32 j = 0; j < object->polygon_len; j++)
            {
                object->polygons[j].x = chunk_read_f32(&reader);
                object->polygons[j].y = chunk_read_f32(&reader);
            }
        }

        into[*current_layer] = layer;
        (*current_layer)++;
    }
    else if (IDEQ(chunk.id, "GLYR"))
    {
        MapLayer layer;
        layer.name = chunk_read_string(&reader);
        layer.class_name = chunk_read_string(&reader);
        layer.data.group.layer_len = chunk_read_u32(&reader);
        layer.type = Layer_Group;

        u32 current_group_layer = 0;
        layer.data.group.layers =
            malloc(sizeof(MapLayer) * layer.data.group.layer_len);
        for (u32 i = 0; i < layer.data.group.layer_len; i++)
            parse_map_layers(inff, map, layer.data.group.layers,
                             &current_group_layer, chunk_index);

        into[*current_layer] = layer;
        (*current_layer)++;
    }
    else if (IDEQ(chunk.id, "ILYR"))
    {
        MapLayer layer;
        layer.name = chunk_read_string(&reader);
        layer.class_name = chunk_read_string(&reader);
        layer.data.image.image_path = chunk_read_string(&reader);
        layer.data.image.parallax_x = chunk_read_f32(&reader);
        layer.data.image.parallax_y = chunk_read_f32(&reader);
        layer.data.image.offset_x = chunk_read_f32(&reader);
        layer.data.image.offset_y = chunk_read_f32(&reader);
        layer.type = Layer_Image;

        into[*current_layer] = layer;
        (*current_layer)++;
    }
    else
    {
        FATAL("Unknown chunk id: %s", chunk.id);
    }
}

void parse_map_from(Map *map, INFF *inff)
{
    INFFChunk info = inff->chunks[0];
    if (IDNEQ(info.id, "INFO"))
        FATAL("INFO chunk not found in map file");

    // read map properties
    ChunkDataReader reader = {0, info.data};
    map->width = chunk_read_u32(&reader);
    map->height = chunk_read_u32(&reader);
    map->layer_len = chunk_read_u32(&reader);
    map->layers = malloc(sizeof(MapLayer) * map->layer_len);

    u32 current_layer = 0;
    for (u32 i = 1; i < inff->num_chunks;)
        parse_map_layers(inff, map, map->layers, &current_layer, &i);
}

void map_layer_free(MapLayer *layer)
{
    free(layer->name);
    free(layer->class_name);

    switch (layer->type)
    {
    case Layer_Tile:
        free(layer->data.tile.tiles);
        break;
    case Layer_Object:
        for (u32 i = 0; i < layer->data.object.object_len; i++)
        {
            Object *object = &layer->data.object.objects[i];
            free(object->name);
            free(object->class_name);
            if (object->polygons)
                free(object->polygons);
        }
        free(layer->data.object.objects);
        break;
    case Layer_Image:
        free(layer->data.image.image_path);
        break;
    case Layer_Group:
        for (u32 i = 0; i < layer->data.group.layer_len; i++)
        {
            map_layer_free(&layer->data.group.layers[i]);
        }
        free(layer->data.group.layers);
        break;
    }
}

void map_free(Map *map)
{
    free(map->tileset.name);
    free(map->tileset.image_path);
    for (u32 i = 0; i < map->layer_len; i++)
    {
        map_layer_free(&map->layers[i]);
    }
    free(map->layers);
}
