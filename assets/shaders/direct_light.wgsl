struct VertexOutput {
  @builtin(position) position: vec4f,
}

struct PushConstants {
  color: vec3f,
  intensity: f32,
  volumetric_intensity: f32,

  // FIXME vec3f jank
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
    let mask_size = screen_size * 16.0;
    let tex_coords = in.position.xy / screen_size;

    let base_color = textureSample(color, tex_sampler, tex_coords);
    var color = base_color.rgb;

    var light_color = push_constants.color;
    // z = 1.0 indicates that the mask texture is enabled
    if push_constants.mask_tex_offset.z == 1.0 {
        let mask_tex_coords = (in.position.xy + push_constants.mask_tex_offset.xy) / mask_size;
        let mask = textureSample(shadow, tex_sampler, mask_tex_coords);
        light_color *= 1.0 - mask.r;
    }

    if base_color.a < 0.1 {
        color = light_color * push_constants.volumetric_intensity;
    }

    return vec4f(color * light_color * push_constants.intensity, 1.0);
}