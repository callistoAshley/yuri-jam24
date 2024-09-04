struct VertexInput {
  @location(0) position: vec2f,
  @location(1) tex_coords: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
}


struct PushConstants {
    color: vec3f,
    camera_position: vec2f,
    position: vec2f,
    radius: f32,
    scale: f32,
    solid: u32,
}

var<push_constant> push_constants: PushConstants;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;
  out.position = vec4f(in.position, 0.0, 1.0);
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let frag_position = in.position.xy + push_constants.camera_position * push_constants.scale;
  let circle_position = push_constants.position * 8 * push_constants.scale;
  let distance = distance(frag_position, circle_position);
  let scaled_radius = push_constants.radius * push_constants.scale * 8;

  if (scaled_radius < distance) {
    discard;
  }

  var alpha = 1.0;
  if (push_constants.solid == 1u) {
    alpha = 1.0 - (distance - scaled_radius) / 8.0;
  }

  let out = vec4f(push_constants.color, alpha);

  return out;
}