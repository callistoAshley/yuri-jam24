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
var<push_constant> transform_index: u32;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let transform = transforms[transform_index];
  let world_position = transform * vec4f(in.position, 0.0, 1.0);

  let position = world_position.xy / vec2f(640.0, 480.0) * 2.0 - 1.0;
  let inverted_y = vec2f(position.x, -position.y);

  out.position = vec4f(inverted_y, 0.0, 1.0);
  out.tex_coords = in.tex_coords;
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  return vec4f(in.tex_coords, 1.0, 1.0);
}