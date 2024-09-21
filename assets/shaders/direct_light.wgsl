struct VertexOutput {
  @builtin(position) position: vec4f,
}

struct PushConstants {
  color: vec3f,
  angle: f32,
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
    out.position = vec4(positions[vertex_index], 0.0, 1.0);

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let screen_size = vec2f(160.0, 90.0);
    let tex_coords = in.position.xy / screen_size;

    let mask_tex_coords = tex_coords / 16.0;

    var color = textureSample(color, tex_sampler, tex_coords);
    var mask = textureSample(shadow, tex_sampler, mask_tex_coords);

    var light_color = push_constants.color;
    if mask.r > 0.1 {
        light_color *= 0.5;
    }

    if color.a < 0.1 {
        color = vec4f(0.05, 0.05, 0.05, 1.0);
    }

    return vec4f(color.rgb * light_color, 1.0);
}