struct VertexInput {
  @location(0) position: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
}


struct PushConstants {
    color: vec3f,
    camera_position: vec2f,
    position: vec2f,
    rotation: vec2f,
    scale: f32,
    solid: u32,
}

var<push_constant> push_constants: PushConstants;

const SCREEN_SIZE: vec2f = vec2f(320.0, 180.0);

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    let s = push_constants.rotation.x;
    let c = push_constants.rotation.y;
    let rotated_position = vec2(
        in.position.x * c - in.position.y * s,
        -(in.position.x * s + in.position.y * c)
    );

  // in box2d, 1m = 16 pixels
  // HOWEVER the game world is rendered to the screen at push_constants.scale
  // the camera coordinates are also at this lower scale, so we need to scale up that too
  //
  // so all in all, we need to: offset the vertex position by the object's position, offset by the camera position, and scale it up by 16 * push_constants.scale
  // HOWEVER we also need to output normalized device coordinates, so we need to divide by the screen size too (which is SCREEN_SIZE * push_constants.scale)
    let screen_size = SCREEN_SIZE * push_constants.scale;
    let scaled_to_camera_position = (rotated_position + push_constants.position) * 16.0;
    let internal_px_position = scaled_to_camera_position - push_constants.camera_position;
    let screen_position = internal_px_position * push_constants.scale;
    let normalized_position = screen_position / screen_size;
    let ndc = normalized_position * 2.0 - 1.0;
    out.position = vec4f(ndc.x, -ndc.y, 0.0, 1.0);

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var alpha = 0.1;
    if push_constants.solid == 1u {
        alpha = 0.25;
    }

    let out = vec4f(push_constants.color, alpha);

    return out;
}