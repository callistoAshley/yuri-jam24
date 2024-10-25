struct VertexInput {
  @location(0) position: vec3f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) center_position: vec2f,
}

@group(0) @binding(0)
var<storage> transforms: array<mat4x4f>;

struct PushConstants {
  camera: mat4x4f,
  light_position: vec2f,
  offset: vec2f,
  transform_index: u32,

  viewport_offset: vec2f,
  camera_position: vec2f,
  radius: f32
}

var<push_constant> push_constants: PushConstants;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    let transform = transforms[push_constants.transform_index];

    let base_world_position = transform * vec4f(in.position.xy - push_constants.offset, 0.0, 1.0);
    let world_position = vec4(base_world_position.xy - in.position.z * push_constants.light_position, 0, 1 - in.position.z);
    let camera_position = push_constants.camera * world_position;
    out.position = camera_position;

    let center_world_position = transform * vec4f(0.0, 0.0, 0.0, 1.0);
    out.center_position = center_world_position.xy - push_constants.camera_position;

    return out;
}

struct FragmentOutput {
  @location(0) color: vec4f,
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;

    if push_constants.radius != -1.0 {
        let frag_position = in.position.xy - push_constants.viewport_offset;
        let distance = distance(frag_position, in.center_position);
        out.color = vec4(1.0 - distance / push_constants.radius, 0.0, 0.0, 0.0);
    } else {
    // we're only outputting the red channel
        out.color = vec4(0.5, 0.0, 0.0, 0.0);
    }

    return out;
}