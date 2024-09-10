struct VertexInput {
  @location(0) position: vec2f,
  @location(1) tex_coords: vec2f,
  @builtin(vertex_index) vertex_index: u32,
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
}

var<push_constant> push_constants: PushConstants;

const NORMALS: array<vec2f, 4> = array(
    vec2f(1.0, 1.0),
    vec2f(-1.0, 1.0),
    vec2f(-1.0, -1.0),
    vec2f(1.0, -1.0),
);

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    let transform = transforms[push_constants.transform_index];
    let world_position = transform * vec4f(in.position, 0.0, 1.0);
    out.position = push_constants.camera * world_position;

    out.tex_coords = in.tex_coords;

    var normals = NORMALS;
    out.normal = normals[in.vertex_index % 4];

    return out;
}

struct FragmentOutput {
  @location(0) color: vec4f,
  @location(1) normals: vec4f,
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;
    let texture = textures[push_constants.texture_index];
    let color = textureSample(texture, tex_sampler, in.tex_coords);

    if color.a < 0.1 {
      discard;
    }
    out.color = color;
    out.normals = vec4f(in.normal, 1.0, 1.0);

    return out;
}