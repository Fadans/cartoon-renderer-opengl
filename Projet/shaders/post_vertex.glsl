#version 450

out vec2 frag_uv;

void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    frag_uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
