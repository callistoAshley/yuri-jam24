struct VertexInput {
  @location(0) position: vec2f,
  // We don't use this, but it's needed to match the vertex layout
  @location(1) tex_coords_unused: vec2f,
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
  camera_world_pos: vec3f,
  transform_index: u32,
  light_world_pos: vec3f,
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
  let color = textureSample(color, tex_sampler, in.tex_coords);
  let normal = textureSample(normal, tex_sampler, in.tex_coords);

  let ambient_strength = 0.1;
  let ambient = ambient_strength * push_constants.color;

  let screen_size = vec2f(640.0, 480.0);

  let constant = 1.0;
  let linear = 0.027;
  let quadratic = 0.0028;

  let frag_world_pos = vec3f(in.tex_coords * screen_size, 0.0) + push_constants.camera_world_pos;
  let distance = length(push_constants.light_world_pos - frag_world_pos);
  let attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
  
  let out = color * push_constants.color * attenuation;

  return vec4f(out.rgb, 1.0);
}