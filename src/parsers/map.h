#pragma once

#include <cglm/struct.h>
#include "inff.h"
#include "sensible_nums.h"

typedef struct MapLayer MapLayer;

typedef enum
{
    Obj_Rectangle,
    Obj_Polygon,
    Obj_Ellipse,
    Obj_Text,
    Obj_Point,
} ObjectType;

typedef struct
{
    char *name;
    char *class_name;

    f32 x, y;
    f32 width, height;

    ObjectType type;

    u32 polygon_len;
    vec2s *polygons;
} Object;

typedef enum
{
    Layer_Tile,
    Layer_Object,
    Layer_Image,
    Layer_Group,
} LayerType;

typedef union
{
    struct
    {
        f32 parallax_x, parallax_y;
        i32 *tiles;
    } tile;
    struct
    {
        u32 object_len;
        Object *objects;
    } object;
    struct
    {
        u32 layer_len;
        MapLayer *layers;
    } group;
    struct
    {
        char *image_path;
        f32 parallax_x, parallax_y;
    } image;
} LayerData;

typedef struct MapLayer
{
    char *name;
    char *class_name;
    LayerType type;
    LayerData data;
} MapLayer;

typedef struct
{
    u32 width, height;

    struct
    {
        char *name;
        char *image_path;
    } tileset;

    u32 layer_len;
    MapLayer *layers;
} Map;

// NOTE: the map duplicates the data from the inff, so you can free the inff
// after parsing the map
void parse_map_from(Map *map, INFF *inff);
void map_free(Map *map);
