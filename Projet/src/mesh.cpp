#include "mesh.hh"
#include <cmath>

static const float PI = 3.14159265f;

Mesh::~Mesh() {
    if (vbo_pos) glDeleteBuffers(1, &vbo_pos);
    if (vbo_nor) glDeleteBuffers(1, &vbo_nor);
    if (vbo_uv)  glDeleteBuffers(1, &vbo_uv);
    if (vao_id)  glDeleteVertexArrays(1, &vao_id);
}

// ─── helpers internes ─────────────────────────────────────────────────────

static void push3(std::vector<GLfloat>& v, float a, float b, float c) {
    v.push_back(a); v.push_back(b); v.push_back(c);
}
static void push2(std::vector<GLfloat>& v, float a, float b) {
    v.push_back(a); v.push_back(b);
}

// ─── sphère UV ────────────────────────────────────────────────────────────
// rings  = divisions latitude (nord → sud)
// sectors = divisions longitude (autour de Y)
// P(phi, theta) = R*(sin phi*cos theta, cos phi, sin phi*sin theta)
// normale = P/R (sphère unitaire normalisée = position)
// UV : u = theta/2π, v = phi/π
Mesh* Mesh::make_sphere(int rings, int sectors, float radius) {
    Mesh* m = new Mesh();

    for (int r = 0; r < rings; r++) {
        float phi0 = PI * r       / rings;
        float phi1 = PI * (r + 1) / rings;

        for (int s = 0; s < sectors; s++) {
            float theta0 = 2*PI * s       / sectors;
            float theta1 = 2*PI * (s + 1) / sectors;

            // 4 coins du quad
            float x00=std::sin(phi0)*std::cos(theta0), y00=std::cos(phi0), z00=std::sin(phi0)*std::sin(theta0);
            float x10=std::sin(phi1)*std::cos(theta0), y10=std::cos(phi1), z10=std::sin(phi1)*std::sin(theta0);
            float x11=std::sin(phi1)*std::cos(theta1), y11=std::cos(phi1), z11=std::sin(phi1)*std::sin(theta1);
            float x01=std::sin(phi0)*std::cos(theta1), y01=std::cos(phi0), z01=std::sin(phi0)*std::sin(theta1);

            float u0=(float)s/sectors, u1=(float)(s+1)/sectors;
            float v0=(float)r/rings,   v1=(float)(r+1)/rings;

            // Triangle 1 : A(r,s), D(r,s+1), C(r+1,s+1)  — CCW vu de l'extérieur
            push3(m->positions, x00*radius, y00*radius, z00*radius);
            push3(m->normals,   x00, y00, z00);
            push2(m->uvs, u0, v0);

            push3(m->positions, x01*radius, y01*radius, z01*radius);
            push3(m->normals,   x01, y01, z01);
            push2(m->uvs, u1, v0);

            push3(m->positions, x11*radius, y11*radius, z11*radius);
            push3(m->normals,   x11, y11, z11);
            push2(m->uvs, u1, v1);

            // Triangle 2 : A(r,s), C(r+1,s+1), B(r+1,s)
            push3(m->positions, x00*radius, y00*radius, z00*radius);
            push3(m->normals,   x00, y00, z00);
            push2(m->uvs, u0, v0);

            push3(m->positions, x11*radius, y11*radius, z11*radius);
            push3(m->normals,   x11, y11, z11);
            push2(m->uvs, u1, v1);

            push3(m->positions, x10*radius, y10*radius, z10*radius);
            push3(m->normals,   x10, y10, z10);
            push2(m->uvs, u0, v1);
        }
    }
    return m;
}

// ─── cylindre ouvert ──────────────────────────────────────────────────────
Mesh* Mesh::make_cylinder(int n, float radius, float height) {
    Mesh* m = new Mesh();
    for (int i = 0; i < n; i++) {
        float t0=2*PI*i/n, t1=2*PI*(i+1)/n;
        float x0=radius*std::cos(t0), z0=radius*std::sin(t0);
        float x1=radius*std::cos(t1), z1=radius*std::sin(t1);
        float u0=(float)i/n, u1=(float)(i+1)/n;

        // Triangle 1 : A, D, C
        push3(m->positions,x0,-height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,0);
        push3(m->positions,x0, height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,1);
        push3(m->positions,x1, height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,1);
        // Triangle 2 : A, C, B
        push3(m->positions,x0,-height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,0);
        push3(m->positions,x1, height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,1);
        push3(m->positions,x1,-height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,0);
    }
    return m;
}

// ─── upload GPU ───────────────────────────────────────────────────────────
// Utilise les locations fixes (layout qualifiers dans les shaders)
void Mesh::upload() {
    if (uploaded_) return;

    glGenVertexArrays(1, &vao_id);
    glBindVertexArray(vao_id);

    auto vbo_upload = [](GLuint& id, const std::vector<GLfloat>& data, GLint loc, int n) {
        glGenBuffers(1, &id);
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(GLfloat), data.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(loc, n, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(loc);
    };

    vbo_upload(vbo_pos, positions, ATTR_POSITION, 3);
    vbo_upload(vbo_nor, normals,   ATTR_NORMAL,   3);
    vbo_upload(vbo_uv,  uvs,       ATTR_UV,       2);

    glBindVertexArray(0);
    uploaded_ = true;
}

void Mesh::draw() const {
    glBindVertexArray(vao_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)positions.size() / 3);
    glBindVertexArray(0);
}
