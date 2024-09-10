struct VertexInput {
  @location(0) tile_id: i32,
  @builtin(vertex_index) vertex_index: u32,
  @builtin(instance_index) instance_index: u32,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) tex_coords: vec2f,
  @location(1) normal: vec2f,
}

@group(0) @binding(0)
var<storage> transforms: array<mat4x4f>;
@group(0) @binding(1)
var textures: binding_array<texture_2d<f32>>;
@group(0) @binding(2)
var tex_sampler: sampler;

struct PushConstants {
  camera: mat4x4f,
  transform_index: u32,
  texture_index: i32,
  map_width: u32,
}

var<push_constant> push_constants: PushConstants;

const VERTEX_POSITIONS = array<vec2f, 6>(
    vec2f(0.0, 0.0),
    vec2f(8.0, 0.0),
    vec2f(0.0, 8.0),
    vec2f(8.0, 0.0),
    vec2f(0.0, 8.0),
    vec2f(8.0, 8.0),
);
const TEX_COORDS = array<vec2f, 6>(
    // slightly smaller than 8x8 to reduce bleeding from adjacent pixels
    vec2f(0.01, 0.01),
    vec2f(7.99, 0.01),
    vec2f(0.01, 7.99),
    vec2f(7.99, 0.01),
    vec2f(0.01, 7.99),
    vec2f(7.99, 7.99),
);
const NORMALS: array<vec2f, 6> = array(
    vec2f(1.0, 1.0),
    vec2f(-1.0, 1.0),
    vec2f(1.0, -1.0),
    vec2f(-1.0, 1.0),
    vec2f(1.0, -1.0),
    vec2f(-1.0, -1.0),
);

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;

    if input.tile_id == -1 {
        return output;
    }
    let tile_id = u32(input.tile_id);

    let tile_position = vec2f(
        f32(input.instance_index % push_constants.map_width),
        f32(input.instance_index / push_constants.map_width)
    );
    var vertex_positions = VERTEX_POSITIONS;
    let vertex_position = vertex_positions[input.vertex_index] + (tile_position * 8.0);

    let transform = transforms[push_constants.transform_index];
    let world_position = transform * vec4f(vertex_position, 0.0, 1.0);

    output.position = push_constants.camera * world_position;

    let tileset = textures[push_constants.texture_index];

    var vertex_tex_coords = TEX_COORDS;
    let tex_size = textureDimensions(tileset);
    let tileset_width = tex_size.x / 8;

    let vertex_tex_coord = vertex_tex_coords[input.vertex_index];
    let tex_offset = vec2f(
        f32(tile_id % tileset_width),
        f32(tile_id / tileset_width)
    ) * 8.0;
    output.tex_coords = (vertex_tex_coord + tex_offset) / vec2f(tex_size);

    var normals = NORMALS;
    output.normal = normals[input.vertex_index];

    return output;
}

struct FragmentOutput {
  @location(0) color: vec4f,
  @location(1) normals: vec4f,
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    var output: FragmentOutput;

    let tileset = textures[push_constants.texture_index];
    let color = textureSample(tileset, tex_sampler, in.tex_coords);

    if color.a < 0.1 {
      discard;
    }
    output.color = color;

    output.normals = vec4f(in.normal, 1.0, 1.0);

    return output;
}