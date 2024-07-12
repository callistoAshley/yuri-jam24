struct VertexInput {
  @location(0) data: vec4f, // not actually vec4, but vec3 + u32
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) color: vec3f,
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;
  let position = in.data.xyz / vec3f(80.0, 60.0, 1.0);
  out.position = vec4f(position, 1.0);

  let color = bitcast<u32>(in.data.w);
  out.color = vec3f(
    f32((color >> 16) & 0xFF) / 255.0,
    f32((color >> 8) & 0xFF) / 255.0,
    f32(color & 0xFF) / 255.0
  );
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  return vec4f(in.color, 1.0);
}