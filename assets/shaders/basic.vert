#version 460 core // opengl 4.6

in vec3 position;

void main() {
    gl_Position = vec4(position, 1.0);
}