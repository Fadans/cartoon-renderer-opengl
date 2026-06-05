#version 450

layout(location = 0) in vec3 position;

uniform mat4 mvp;
uniform mat4 world;

out vec2 ground_xz;

void main() {
    vec4 w = world * vec4(position, 1.0);
    ground_xz = w.xz / w.w;
    gl_Position = mvp * vec4(position, 1.0);
}
