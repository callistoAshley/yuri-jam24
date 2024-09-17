struct VertexInput {
  @location(0) position: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
}

@group(0) @binding(0)
var<storage> transforms: array<mat4x4f>;

struct PushConstants {
  camera: mat4x4f,
  transform_index: u32,
  tile_x: u32,
  tile_y: u32,
}

var<push_constant> push_constants: PushConstants;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    let tile_position = vec2f(
        f32(push_constants.tile_x * 8),
        f32(push_constants.tile_y * 8)
    );
    let vertex_position = in.position + tile_position;

    let transform = transforms[push_constants.transform_index];
    let world_position = transform * vec4f(vertex_position, 0.0, 1.0);
    out.position = push_constants.camera * world_position;

    return out;
}

struct FragmentOutput {
  @location(0) color: vec4f,
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;

    out.color = vec4f(1.0, 1.0, 1.0, 1.0);

    return out;
}