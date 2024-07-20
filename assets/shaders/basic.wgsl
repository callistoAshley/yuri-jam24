struct VertexInput {
  @location(0) position: vec2f,
  @location(1) tex_coords: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) tex_coords: vec2f,
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

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let transform = transforms[push_constants.transform_index];
  let world_position = transform * vec4f(in.position, 0.0, 1.0);

  out.position = push_constants.camera * world_position;
  out.tex_coords = in.tex_coords;
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let texture = textures[push_constants.texture_index];
  let color = textureSample(texture, tex_sampler, in.tex_coords);

  if (color.a < 0.1) {
    discard;
  }

  return color;
}