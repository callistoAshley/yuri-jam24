struct VertexOutput {
  @builtin(position) position: vec4f,
}

struct PushConstants {
  color: vec3f,
  intensity: f32,
  volumetric_intensity: f32,
}

var<push_constant> push_constants: PushConstants;

@group(0) @binding(0)
var color: texture_2d<f32>;
@group(0) @binding(1)
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
    out.position = vec4(positions[vertex_index], 0.0, 1.0);

    return out;
}

const SCREEN_SIZE: vec2f = vec2f(320.0, 180.0);

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let mask_size = SCREEN_SIZE * 8.0;
    let tex_coords = in.position.xy / SCREEN_SIZE;

    let base_color = textureSample(color, tex_sampler, tex_coords);
    var color = base_color.rgb;

    let light_color = push_constants.color;
    if base_color.a < 0.1 {
        color = light_color * push_constants.volumetric_intensity;
    }

    return vec4f(color * light_color * push_constants.intensity, 1.0);
}