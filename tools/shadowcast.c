#include "sensible_nums.h"
#include "utility/macros.h"
#include "utility/vec.h"

#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_iostream.h>
#include <stdio.h>
#include <stdbool.h>

#define EXPECTED_ARGC 3
#define MAX_ARGC 5

typedef struct
{
    f32 x, y;
} Point;

typedef struct
{
    SDL_Surface *surface;
    u32 cell_width, cell_height;
} Image;

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
    Image image = {surf, cell_width, cell_height};

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

    // cells are laid out by row, column
    for (i32 cell_y = 0; cell_y < cell_count_y; ++cell_y)
    {
        for (i32 cell_x = 0; cell_x < cell_count_x; ++cell_x)
        {
            // use marching squares to find the outline of the cell

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

            u8 *cases = malloc(cell_width * cell_height);
            i32 cell_px = cell_x * cell_width;
            i32 cell_py = cell_y * cell_height;
            for (u32 y = 0; y < cell_height; y++)
            {
                for (u32 x = 0; x < cell_width; x++)
                {
                    u8 current = 0;
                    for (int i = 0; i < 4; i++)
                    {
                        i32 px = cell_px + x + OFFSETS[i][0];
                        i32 py = cell_py + y + OFFSETS[i][1];

                        if (px < 0 || px >= surf->w || py < 0 || py >= surf->h)
                        {
                            current = current << 1;
                            continue;
                        }

                        if (px < cell_px || px >= cell_px + cell_width ||
                            py < cell_py || py >= cell_py + cell_height)
                        {
                            current = current << 1;
                            continue;
                        }

                        u8 alpha = 0;
                        SDL_ReadSurfacePixel(surf, px, py, NULL, NULL, NULL,
                                             &alpha);
                        current = current << 1 | (alpha > 0);
                    }
                    cases[y * cell_width + x] = current;
                }
            }

            // now that we've created all our cases, we can construct the lines
            // that make up the outline of the cell
            vec lines;
            vec_init(&lines, sizeof(Point));

            // now handle all of the marching squares cases
            for (i32 x = 0; x < cell_width; x++)
            {
                for (i32 y = 0; y < cell_height; y++)
                {
                    u8 current = cases[x + y * cell_width];
                    u32 case_point_count = CASE_POINT_COUNTS[current];
                    for (u32 i = 0; i < case_point_count; i++)
                    {
                        Point point = CASES[current][i];
                        point.x += x;
                        point.y += y;
                        vec_push(&lines, &point);
                    }
                }
            }

            // pretty print the lines
            for (u32 i = 0; i < lines.len; i++)
            {
                Point *point = vec_get(&lines, i);
                printf("%f, %f\n", point->x + cell_x * cell_width,
                       -point->y - cell_y * cell_height);
            }
        }
    }
}