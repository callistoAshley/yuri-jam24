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
  // min, max angle in radians
  // these are angles on a circle, so they are in the range of 0 to 2 * PI
  // we want the light to only be visible inside the min and max angle
  angle: vec2f,
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

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    var out: VertexOutput;

    var positions = POSITIONS;
    let position = positions[vertex_index] * push_constants.radius;

    let translated_position = position + push_constants.position - push_constants.camera_position;
    let scaled_position = translated_position / vec2f(160.0, 90.0);
    let normalized_position = scaled_position * 2.0 - 1.0;

    out.position = vec4f(normalized_position.x, -normalized_position.y, 0.0, 1.0);

    return out;
}

const SCREEN_SIZE: vec2f = vec2f(160.0, 90.0);
const PI: f32 = radians(180.0);

// if angle is outside of the range of min..max this function returns 0
// the closer angle gets to min or max, the lower a value this function returns
// at exactly halfway between min and max, this function returns 1.0
//
// the output looks like this:
// 0 | angle < min
// 0..0.99 | angle < (max - min / 2)
// 1.0 | angle == (max - min /2)
// 0..0.99 | angle > (max - min / 2)
// 0 | angle > max
fn angular_falloff(min: f32, max: f32, angle: f32) -> f32 {
    if min == radians(-180.0) && max == radians(180.0) {
        return 1.0;
    }

    let angle_diff = max - min;
    let half_angle = min + angle_diff / 2.0;
    let angle_normalized = (angle - min) / angle_diff;
    let falloff = 1.0 - abs(angle_normalized - 0.5) * 2.0;
    let angular_falloff = clamp(falloff, 0.0, 1.0);
    return angular_falloff;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let frag_world_coord = in.position.xy + push_constants.camera_position;

    let tex_coords = in.position.xy / SCREEN_SIZE;

    let color = textureSample(color, tex_sampler, tex_coords);

    let dist = distance(push_constants.position, frag_world_coord);
    let dist_norm = dist / push_constants.radius;
    let dir = normalize(push_constants.position - frag_world_coord);

    // convert angle from -PI..PI to 0..2*PI
    let angle = atan2(dir.y, dir.x);

    // we don't render a perfect circle (it's a square), so we need to clamp the factor
    let radial_falloff = pow(clamp(1.0 - dist_norm, 0.0, 1.0), 2.0);

    let angular_falloff = angular_falloff(push_constants.angle.x, push_constants.angle.y, angle);


    let final_intensity = push_constants.intensity * angular_falloff * radial_falloff;
    let light_color = push_constants.color * final_intensity;

    let shaded_color = color.rgb * light_color;
    var volumetric_color = vec3f(0.0);
    if color.a < 0.1 {
        volumetric_color = light_color * push_constants.volumetric_intensity;
    }

    return vec4f(shaded_color + volumetric_color, 1.0);
}