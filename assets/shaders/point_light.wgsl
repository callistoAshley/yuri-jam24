struct VertexOutput {
  @builtin(position) position: vec4f,
}

struct PushConstants {
  color: vec3f,

  position: vec2f,
  camera_position: vec2f,

  intensity: f32,
  radius: f32,
  volumetric_intensity: f32,
  
  mask_tex_offset: vec3f,
}

var<push_constant> push_constants: PushConstants;

@group(0) @binding(0)
var color: texture_2d<f32>;
@group(0) @binding(1)
var shadow: texture_2d<f32>;
@group(0) @binding(2)
var tex_sampler: sampler;

const POSITIONS: array<vec2f, 6> = array<vec2f, 6>(
    vec2f(-1.0, 1.0),
    vec2f(1.0, 1.0),
    vec2f(-1.0, -1.0),
    vec2f(-1.0, -1.0),
    vec2f(1.0, 1.0),
    vec2f(1.0, -1.0),
);

const SCREEN_SIZE: vec2f = vec2f(320.0, 180.0);
const PI: f32 = radians(180.0);

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    var out: VertexOutput;

    var positions = POSITIONS;
    let position = positions[vertex_index] * push_constants.radius;

    let translated_position = position + push_constants.position - push_constants.camera_position;
    let scaled_position = translated_position / SCREEN_SIZE;
    let normalized_position = scaled_position * 2.0 - 1.0;

    out.position = vec4f(normalized_position.x, -normalized_position.y, 0.0, 1.0);

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let frag_world_coord = in.position.xy + push_constants.camera_position;

    let tex_coords = in.position.xy / SCREEN_SIZE;
    let mask_size = SCREEN_SIZE * 16.0;

    let color = textureSample(color, tex_sampler, tex_coords);

    let dist = distance(push_constants.position, frag_world_coord);
    let dist_norm = dist / push_constants.radius;
    let dir = normalize(push_constants.position - frag_world_coord);

    // convert angle from -PI..PI to 0..2*PI
    let angle = atan2(dir.y, dir.x);

    // we don't render a perfect circle (it's a square), so we need to clamp the factor
    let radial_falloff = pow(clamp(1.0 - dist_norm, 0.0, 1.0), 2.0);

    let final_intensity = push_constants.intensity * radial_falloff;
    var light_color = push_constants.color * final_intensity;

    // z = 1.0 indicates that the mask texture is enabled
    if push_constants.mask_tex_offset.z == 1.0 {
        let mask_tex_coords = (in.position.xy + push_constants.mask_tex_offset.xy) / mask_size;
        let mask = textureSample(shadow, tex_sampler, mask_tex_coords);
        light_color *= 1.0 - mask.r;
    }

    let shaded_color = color.rgb * light_color;
    var volumetric_color = vec3f(0.0);
    if color.a < 0.1 {
        volumetric_color = light_color * push_constants.volumetric_intensity;
    }

    return vec4f(shaded_color + volumetric_color, 1.0);
}