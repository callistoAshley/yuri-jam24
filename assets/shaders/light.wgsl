struct VertexInput {
  @location(0) position: vec2f,
  @location(1) unused: vec2f, // We don't use this, but it's needed to match the vertex layout
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) tex_coords: vec2f,
}

@group(0) @binding(0)
var<storage> transforms: array<mat4x4f>;
@group(0) @binding(1)
var color: texture_2d<f32>;
@group(0) @binding(2)
var normal: texture_2d<f32>;
@group(0) @binding(3)
var tex_sampler: sampler;

struct PushConstants {
  camera: mat4x4f,
  transform_index: u32,
  color: vec4f,
}

var<push_constant> push_constants: PushConstants;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let transform = transforms[push_constants.transform_index];
  let world_position = transform * vec4f(in.position, 0.0, 1.0);

  out.position = push_constants.camera * world_position;
  // we want -1,1 to map to 0,0 and 1,-1 to map to 1,1
  let tex_x = (out.position.x + 1.0) / 2.0; // (-1 + 1) / 2 = 0, (1 + 1) / 2 = 1
  let tex_y = (out.position.y - 1.0) / -2.0; // (1 - 1) / -2 = 0, (-1 - 1) / -2 = 1
  out.tex_coords = vec2f(tex_x, tex_y);
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  // don't bother performing expensive lighting calculations if the tex_coords are out of bounds
  if (in.tex_coords.x < 0.0 || in.tex_coords.x > 1.0 || in.tex_coords.y < 0.0 || in.tex_coords.y > 1.0) {
    discard;
  }

  let color = textureSample(color, tex_sampler, in.tex_coords);
  let normal = textureSample(normal, tex_sampler, in.tex_coords);

  return color;
}