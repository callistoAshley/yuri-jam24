#pragma once

#include "cglm/struct/vec2.h"
#include <cglm/struct.h>
#include <stdbool.h>

#include "sensible_nums.h"

typedef mat4s Viewport;

typedef struct
{
    vec3s position;
    vec3s scale;
    versors rotation; // quaternion
} Transform;

// i *would* make this a #define but -Wpedantic doesn't like it
static const Transform TRANSFORM_UNIT = {
    .position = GLMS_VEC3_ZERO_INIT,
    .scale = GLMS_VEC3_ONE_INIT,
    .rotation = GLMS_QUAT_IDENTITY_INIT,
};

Transform transform_from_xyz(float x, float y, float z);
Transform transform_from_pos(vec3s position);
Transform transform_from_rot(versors rotation);
Transform transform_from_scale(vec3s scale);
Transform transform_from_position_scale(vec3s position, vec3s scale);
Transform transform_from_position_rotation(vec3s position, versors rotation);

vec3s transform_local_x(Transform transform);
vec3s transform_local_y(Transform transform);
vec3s transform_local_z(Transform transform);

vec3s transform_left(Transform transform);
vec3s transform_right(Transform transform);
vec3s transform_up(Transform transform);
vec3s transform_down(Transform transform);

vec3s transform_forward(Transform transform);
vec3s transform_backward(Transform transform);

mat4s transform_into_matrix(Transform transform);
Transform transform_from_matrix(mat4s matrix);

typedef struct
{
    vec2s min;
    vec2s max;
} Rect;

Rect rect_init(vec2s min, vec2s max);
Rect rect_from_min_size(vec2s min, vec2s size);
Rect rect_from_size(vec2s size);
Rect rect_from_center_radius(vec2s center, vec2s radius);

#define RECT_UNIT_TEX_COORDS rect_from_size((vec2s){.x = 1, .y = 1})

// c is weird with arrays so we need to pass a pointer
float rect_area(Rect rect);
float rect_width(Rect rect);
float rect_height(Rect rect);
vec2s rect_size(Rect rect);

float rect_top(Rect rect);
float rect_bottom(Rect rect);
float rect_left(Rect rect);
float rect_right(Rect rect);

vec2s rect_center(Rect rect);
vec2s rect_top_left(Rect rect);
vec2s rect_top_right(Rect rect);
vec2s rect_bottom_left(Rect rect);
vec2s rect_bottom_right(Rect rect);

bool rect_contains(Rect rect, vec2s point);
bool rect_contains_other(Rect rect, Rect other);

vec2s rect_clamp(Rect rect, vec2s point);
Rect rect_clip(Rect rect, Rect other);

f32 fclamp(f32 val, f32 min, f32 max);

typedef struct
{
    vec2s position;
    vec2s tex_coords;
} Vertex;

typedef struct
{
    Rect rect;
    Rect tex_coords;
} Quad;

Quad quad_init(Rect rect, Rect tex_coords);
Quad quad_norm_tex_coords(Quad quad, int tex_width, int tex_height);

#define VERTICES_PER_QUAD 6
#define CORNERS_PER_QUAD 4

// Quads are made like this:
// TL------TR
// |  \ /  |
// |  / \  |
// BL-----BR
// this function places the corners in clockwise order starting from the top
// left (TL, TR, BR, BL)
void quad_into_corners(Quad quad, Vertex corners[CORNERS_PER_QUAD]);
Quad quad_from_corners(Vertex vertices[CORNERS_PER_QUAD]);
