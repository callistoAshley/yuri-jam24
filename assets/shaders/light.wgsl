struct VertexInput {
  @location(0) position: vec2f,
  @location(1) _tex_coords: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
}

struct PushConstants {
  color: vec3f,

  position: vec2f,
  camera_position: vec2f,

  radius: f32,
}

var<push_constant> push_constants: PushConstants;

@group(0) @binding(0)
var color: texture_2d<f32>;
@group(0) @binding(1)
var normal: texture_2d<f32>;
@group(0) @binding(2)
var tex_sampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let translated_position = in.position + push_constants.position - push_constants.camera_position;
  let scaled_position = translated_position / vec2f(160.0, 90.0);
  let normalized_position = scaled_position * 2.0 - 1.0;

  out.position = vec4f(normalized_position.x, -normalized_position.y, 0.0, 1.0);

  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let screen_size = vec2f(160.0, 90.0);
  let tex_coords = in.position.xy / screen_size;

  let frag_world_coord = in.position.xy + push_constants.camera_position;
  // we ciel the distance to make the light more "pixelated"
  let dist_raw = distance(frag_world_coord, push_constants.position);
  let dist = ceil(dist_raw / 4) * 4;

  let color = textureSample(color, tex_sampler, tex_coords);

  let light_intensity = 1.0 - dist / push_constants.radius;
  let light_color = vec3f(1.0, 1.0, 1.0) * light_intensity;

  return vec4f(color.rgb * light_color, 1.0);
}