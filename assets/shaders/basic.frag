#version 460 core // opengl 4.6

layout(location = 0) out vec4 diffuseColor;

in vec3 vertexColor;

void main() {
  diffuseColor = vec4(vertexColor, 1.0);
}