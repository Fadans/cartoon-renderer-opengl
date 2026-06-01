#version 450

uniform vec3 outline_color;   // noir par défaut (0,0,0)

layout(location = 0) out vec4 output_color;

void main() {
    output_color = vec4(outline_color, 1.0);
}
