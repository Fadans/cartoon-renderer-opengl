#version 450

// Triangle plein écran généré sans VBO (à partir de gl_VertexID).
// 3 sommets couvrant tout l'écran → pas de géométrie à uploader.
out vec2 frag_uv;

void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    frag_uv = p;                       // (0,0) (2,0) (0,2)
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
