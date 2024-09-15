#include "sensible_nums.h"
#include "utility/macros.h"
#include "utility/vec.h"

#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_iostream.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define EXPECTED_ARGC 3
#define MAX_ARGC 5

typedef struct
{
    f32 x, y;
} Point;
Point cell_center_point;

int point_angular_sort_fn(const void *p1, const void *p2)
{
    Point *point1 = (Point *)p1;
    Point *point2 = (Point *)p2;

    f32 angle1 =
        atan2(point1->y - cell_center_point.y, point1->x - cell_center_point.x);
    f32 angle2 =
        atan2(point2->y - cell_center_point.y, point2->x - cell_center_point.x);

    if (angle1 < angle2)
    {
        return -1;
    }
    else if (angle1 > angle2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// this program creates a list of lines that outline a provided image, for use
// in shadowcasting it can be passed a cell width and height, or it will use the
// image's dimensions as the cell dimensions each cell gets its own list of
// lines, so using a spritesheet with multiple cells is possible
i32 main(i32 argc, char *args[])
{
    if (argc < EXPECTED_ARGC || argc > MAX_ARGC)
    {
        printf("Usage: %s <image> <output filename> | <cell width> <cell "
               "height>\n",
               args[0]);
        return 1;
    }

    // other formats? what are those
    IMG_Init(IMG_INIT_PNG);

    char *image_path = args[1];
    char *output_path = args[2];

    SDL_IOStream *output = SDL_IOFromFile(output_path, "wb");
    SDL_PTR_ERRCHK(output, "failed to open output file");

    SDL_Surface *surf = IMG_Load(image_path);
    SDL_PTR_ERRCHK(surf, "failed to load image");

    i32 cell_width = surf->w;
    i32 cell_height = surf->h;
    if (argc == MAX_ARGC)
    {
        cell_width = atoi(args[3]);
        cell_height = atoi(args[4]);
    }

    cell_center_point = (Point){cell_width / 2.0, cell_height / 2.0};

    // write the magic number
    SDL_WriteIO(output, "SHDW", 4);

    // write the cell dimensions
    // no need to write the cell count, it can be inferred from the image size
    SDL_WriteU32LE(output, cell_width);
    SDL_WriteU32LE(output, cell_height);

    // iterate over the image, cell by cell
    // create a list of points along the edges of the cell
    // and then simplify the list of points, removing any that are redundant

    i32 cell_count_x = surf->w / cell_width;
    i32 cell_count_y = surf->h / cell_height;

    SDL_Surface **surfaces =
        malloc(cell_count_x * cell_count_y * sizeof(SDL_Surface *));
    for (u32 x = 0; x < cell_count_x; x++)
    {
        for (u32 y = 0; y < cell_count_y; y++)
        {
            // create a surface with padding around the cell
            // so that we can use marching squares to find the outline
            SDL_Surface *cell_surf = SDL_CreateSurface(
                cell_width + 2, cell_height + 2, surf->format);
            SDL_Rect src = {x * cell_width, y * cell_height, cell_width,
                            cell_height};
            SDL_Rect dest = {1, 1, cell_width, cell_height};
            SDL_BlitSurface(surf, &src, cell_surf, &dest);
            surfaces[x + y * cell_count_x] = cell_surf;
        }
    }
    u32 padded_cell_width = cell_width + 2;
    u32 padded_cell_height = cell_height + 2;

    // use marching squares to find the outline of the cell
    for (u32 cell_index = 0; cell_index < cell_count_x * cell_count_y;
         cell_index++)
    {
        SDL_Surface *cell = surfaces[cell_index];
        const i32 OFFSETS[][2] = {
            {0, 0},
            {1, 0},
            {1, 1},
            {0, 1},
        };

        // all the marching squares cases
        const Point CASES[16][4] = {
            {}, // no points
            {{0.0, 0.5}, {0.5, 1.0}},
            {{1.0, 0.5}, {0.5, 1.0}},
            {{0.0, 0.5}, {1.0, 0.5}},
            {{0.5, 0.0}, {1.0, 0.5}},
            {{0.0, 0.5}, {0.5, 0.0}, {0.5, 1.0}, {1.0, 0.5}},
            {{0.5, 0.0}, {0.5, 1.0}},
            {{0.0, 0.5}, {0.5, 0.0}},
            {{0.0, 0.5}, {0.5, 0.0}},
            {{0.5, 0.0}, {0.5, 1.0}},
            {{0.0, 0.5}, {0.5, 0.0}, {0.5, 1.0}, {1.0, 0.5}},
            {{0.5, 0.0}, {1.0, 0.5}},
            {{0.0, 0.5}, {1.0, 0.5}},
            {{1.0, 0.5}, {0.5, 1.0}},
            {{0.0, 0.5}, {0.5, 1.0}},
            {}, // no points

        };
        const u32 CASE_POINT_COUNTS[16] = {
            0, 2, 2, 2, 2, 4, 2, 2, 2, 2, 4, 2, 2, 2, 2, 0,
        };

        u8 *cases = malloc(padded_cell_width * padded_cell_height);
        for (u32 y = 0; y < padded_cell_width; y++)
        {
            for (u32 x = 0; x < padded_cell_height + 2; x++)
            {
                u8 current = 0;
                for (int i = 0; i < 4; i++)
                {
                    i32 px = x + OFFSETS[i][0];
                    i32 py = y + OFFSETS[i][1];

                    if (px < 0 || px >= cell->w || py < 0 || py >= cell->h)
                    {
                        current = current << 1;
                        continue;
                    }

                    u8 alpha = 0;
                    SDL_ReadSurfacePixel(cell, px, py, NULL, NULL, NULL,
                                         &alpha);
                    current = current << 1 | (alpha > 0);
                }
                cases[y * padded_cell_width + x] = current;
            }
        }

        // now that we've created all our cases, we can construct the lines
        // that make up the outline of the cell
        vec points;
        vec_init(&points, sizeof(Point));

        // now handle all of the marching squares cases
        for (i32 x = 0; x < padded_cell_width; x++)
        {
            for (i32 y = 0; y < padded_cell_height; y++)
            {
                u8 current = cases[x + y * padded_cell_width];
                u32 case_point_count = CASE_POINT_COUNTS[current];
                for (u32 i = 0; i < case_point_count; i++)
                {
                    Point point = CASES[current][i];
                    point.x += x - 0.5;
                    point.y += y - 0.5;
                    vec_push(&points, &point);
                }
            }
        }

        // order the points in a clockwise fashion
        // this is done by sorting the points by their angle from the center of
        // the cell
        Point center = {cell_width / 2.0, cell_height / 2.0};
        qsort(points.data, points.len, sizeof(Point), point_angular_sort_fn);

        // pretty print the lines
        for (u32 i = 0; i < points.len; i++)
        {
            i32 cell_x = cell_index % cell_count_x * cell_width * 2;
            i32 cell_y = cell_index / cell_count_x * cell_height * 2;
            Point *point = vec_get(&points, i);
            printf("%f, %f\n", point->x + cell_x, -point->y - cell_y);
        }
    }
}