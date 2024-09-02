#include "core_types.h"
#include "cglm/types-struct.h"

Transform transform_from_xyz(float x, float y, float z)
{
    return transform_from_pos((vec3s){.x = x, .y = y, .z = z});
}
Transform transform_from_pos(vec3s position)
{
    return (Transform){
        .position = position,
        .scale = GLMS_VEC3_ONE_INIT,
        .rotation = GLMS_QUAT_IDENTITY_INIT,
    };
}
Transform transform_from_rot(versors rotation)
{
    return (Transform){
        .position = GLMS_VEC3_ZERO_INIT,
        .scale = GLMS_VEC3_ONE_INIT,
        .rotation = rotation,
    };
}
Transform transform_from_scale(vec3s scale)
{
    return (Transform){
        .position = GLMS_VEC3_ZERO_INIT,
        .scale = scale,
        .rotation = GLMS_QUAT_IDENTITY_INIT,
    };
}
Transform transform_from_position_scale(vec3s position, vec3s scale)
{
    return (Transform){
        .position = position,
        .scale = scale,
        .rotation = GLMS_QUAT_IDENTITY_INIT,
    };
}
Transform transform_from_position_rotation(vec3s position, versors rotation)
{
    return (Transform){
        .position = position,
        .scale = GLMS_VEC3_ONE_INIT,
        .rotation = rotation,
    };
}

// ----

vec3s transform_local_x(Transform transform)
{
    return glms_quat_rotatev(transform.rotation, GLMS_XUP);
}
vec3s transform_local_y(Transform transform)
{
    return glms_quat_rotatev(transform.rotation, GLMS_YUP);
}
vec3s transform_local_z(Transform transform)
{
    return glms_quat_rotatev(transform.rotation, GLMS_ZUP);
}

// ----

vec3s transform_left(Transform transform)
{
    return glms_vec3_negate(transform_local_x(transform));
}
vec3s transform_right(Transform transform)
{
    return transform_local_x(transform);
}
vec3s transform_up(Transform transform) { return transform_local_y(transform); }
vec3s transform_down(Transform transform)
{
    return glms_vec3_negate(transform_local_y(transform));
}

// ----

vec3s transform_forward(Transform transform)
{
    return glms_vec3_negate(transform_local_z(transform));
}
vec3s transform_backward(Transform transform)
{
    return transform_local_z(transform);
}

// ----

mat4s transform_into_matrix(Transform transform)
{
    mat4s matrix = glms_mat4_identity();
    matrix = glms_translate(matrix, transform.position);
    matrix = glms_quat_rotate(matrix, transform.rotation);
    matrix = glms_scale(matrix, transform.scale);
    return matrix;
}

Transform transform_from_matrix(mat4s matrix)
{
    Transform transform;

    // Extract translation
    transform.position = glms_vec3(matrix.col[3]);

    // Extract scale from the upper-left 3x3 matrix
    transform.scale.x = glms_vec3_norm(glms_vec3(matrix.col[0]));
    transform.scale.y = glms_vec3_norm(glms_vec3(matrix.col[1]));
    transform.scale.z = glms_vec3_norm(glms_vec3(matrix.col[2]));

    // Remove the scale from the matrix by normalizing the columns
    mat3s rotationMatrix;
    rotationMatrix.col[0] =
        glms_vec3_scale(glms_vec3(matrix.col[0]), 1.0f / transform.scale.x);
    rotationMatrix.col[1] =
        glms_vec3_scale(glms_vec3(matrix.col[1]), 1.0f / transform.scale.y);
    rotationMatrix.col[2] =
        glms_vec3_scale(glms_vec3(matrix.col[2]), 1.0f / transform.scale.z);

    // Extract rotation quaternion from the rotation matrix
    transform.rotation = glms_mat3_quat(rotationMatrix);

    return transform;
}

// ----

Rect rect_init(vec2s min, vec2s max)
{
    Rect rect;
    rect.min = min;
    rect.max = max;
    return rect;
}

Rect rect_from_min_size(vec2s min, vec2s size)
{
    Rect rect;
    rect.min = min;
    rect.max = glms_vec2_add(min, size);
    return rect;
}

Rect rect_centered_from_size(vec2s size)
{
    vec2s half_size = glms_vec2_scale(size, 0.5f);
    return rect_from_min_size(glms_vec2_negate(half_size), size);
}

// ----

float rect_area(Rect rect)
{
    vec2s size = glms_vec2_sub(rect.max, rect.min);
    return size.x * size.y;
}
float rect_width(Rect rect) { return rect.max.x - rect.min.x; }
float rect_height(Rect rect) { return rect.max.y - rect.min.y; }
vec2s rect_size(Rect rect) { return glms_vec2_sub(rect.max, rect.min); }

// ----

float rect_top(Rect rect) { return rect.max.y; }
float rect_bottom(Rect rect) { return rect.min.y; }
float rect_left(Rect rect) { return rect.min.x; }
float rect_right(Rect rect) { return rect.max.x; }

// ----

vec2s rect_center(Rect rect)
{
    return glms_vec2_add(rect.min, glms_vec2_scale(rect_size(rect), 0.5f));
}
vec2s rect_top_left(Rect rect) { return rect.min; }
vec2s rect_top_right(Rect rect)
{
    return (vec2s){.x = rect.max.x, .y = rect.min.y};
}
vec2s rect_bottom_left(Rect rect)
{
    return (vec2s){.x = rect.min.x, .y = rect.max.y};
}
vec2s rect_bottom_right(Rect rect) { return rect.max; }

// ----

bool rect_contains(Rect rect, vec2s point)
{
    return point.x >= rect.min.x && point.x <= rect.max.x &&
           point.y >= rect.min.y && point.y <= rect.max.y;
}

// ----

Quad quad_init(Rect rect, Rect tex_coords)
{
    Quad quad;
    quad.rect = rect;
    quad.tex_coords = tex_coords;
    return quad;
}

Quad quad_norm_tex_coords(Quad quad, int tex_width, int tex_height)
{
    Rect tex_coords = quad.tex_coords;
    vec2s tex_size = (vec2s){.x = tex_width, .y = tex_height};
    tex_coords.min = glms_vec2_div(tex_coords.min, tex_size);
    tex_coords.max = glms_vec2_div(tex_coords.max, tex_size);
    return quad_init(quad.rect, tex_coords);
}

// ----

void quad_into_corners(Quad quad, Vertex corners[4])
{
    corners[0] = (Vertex){
        .position = rect_top_left(quad.rect),
        .tex_coords = rect_top_left(quad.tex_coords),
    };
    corners[1] = (Vertex){
        .position = rect_top_right(quad.rect),
        .tex_coords = rect_top_right(quad.tex_coords),
    };
    corners[2] = (Vertex){
        .position = rect_bottom_right(quad.rect),
        .tex_coords = rect_bottom_right(quad.tex_coords),
    };
    corners[3] = (Vertex){
        .position = rect_bottom_left(quad.rect),
        .tex_coords = rect_bottom_left(quad.tex_coords),
    };
}

void quad_into_vertices(Quad quad, Vertex vertices[6])
{
    Vertex corners[4];
    quad_into_corners(quad, corners);

    // top left
    vertices[0] = corners[0];
    // top right
    vertices[1] = corners[1];
    // bottom left
    vertices[2] = corners[3];

    // top right
    vertices[3] = corners[1];
    // bottom left
    vertices[4] = corners[3];
    // bottom right
    vertices[5] = corners[2];
}

Quad quad_from_vertices(Vertex *vertices)
{
    Rect rect = rect_init(vertices[0].position, vertices[5].position);
    Rect tex_coords = rect_init(vertices[0].tex_coords, vertices[5].tex_coords);
    return quad_init(rect, tex_coords);
}
