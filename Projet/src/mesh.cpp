#include "mesh.hh"
#include <cmath>
#include <array>

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

// ─── cylindre avec caps ───────────────────────────────────────────────────
Mesh* Mesh::make_cylinder(int n, float radius, float height, bool with_caps) {
    Mesh* m = new Mesh();
    for (int i = 0; i < n; i++) {
        float t0=2*PI*i/n, t1=2*PI*(i+1)/n;
        float x0=radius*std::cos(t0), z0=radius*std::sin(t0);
        float x1=radius*std::cos(t1), z1=radius*std::sin(t1);
        float u0=(float)i/n, u1=(float)(i+1)/n;

        // Flanc — Triangle 1 : A, D, C
        push3(m->positions,x0,-height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,0);
        push3(m->positions,x0, height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,1);
        push3(m->positions,x1, height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,1);
        // Flanc — Triangle 2 : A, C, B
        push3(m->positions,x0,-height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,0);
        push3(m->positions,x1, height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,1);
        push3(m->positions,x1,-height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,0);

        if (with_caps) {
            float cu0=0.5f+0.5f*std::cos(t0), cv0=0.5f+0.5f*std::sin(t0);
            float cu1=0.5f+0.5f*std::cos(t1), cv1=0.5f+0.5f*std::sin(t1);
            // Cap haut
            push3(m->positions,0, height/2, 0); push3(m->normals,0,1,0); push2(m->uvs,.5f,.5f);
            push3(m->positions,x1,height/2,z1); push3(m->normals,0,1,0); push2(m->uvs,cu1,cv1);
            push3(m->positions,x0,height/2,z0); push3(m->normals,0,1,0); push2(m->uvs,cu0,cv0);
            // Cap bas
            push3(m->positions,0,-height/2, 0); push3(m->normals,0,-1,0); push2(m->uvs,.5f,.5f);
            push3(m->positions,x0,-height/2,z0); push3(m->normals,0,-1,0); push2(m->uvs,cu0,cv0);
            push3(m->positions,x1,-height/2,z1); push3(m->normals,0,-1,0); push2(m->uvs,cu1,cv1);
        }
    }
    return m;
}

// ─── tore ─────────────────────────────────────────────────────────────────
// R = rayon majeur (centre → centre du tube)
// r = rayon mineur (rayon du tube)
// P(u,v) = ((R + r·cos v)·cos u,  r·sin v,  (R + r·cos v)·sin u)
// Normale = (cos v·cos u,  sin v,  cos v·sin u)
Mesh* Mesh::make_torus(int rings, int sectors, float R, float r) {
    Mesh* m = new Mesh();

    for (int i = 0; i < rings; i++) {
        float u0 = 2*PI * i       / rings;
        float u1 = 2*PI * (i + 1) / rings;

        for (int j = 0; j < sectors; j++) {
            float v0 = 2*PI * j       / sectors;
            float v1 = 2*PI * (j + 1) / sectors;

            auto P = [&](float u, float v) -> std::array<float,3> {
                return { (R + r*std::cos(v))*std::cos(u),
                          r*std::sin(v),
                         (R + r*std::cos(v))*std::sin(u) };
            };
            auto N = [](float u, float v) -> std::array<float,3> {
                return { std::cos(v)*std::cos(u),
                         std::sin(v),
                         std::cos(v)*std::sin(u) };
            };

            // CCW vu de l'extérieur : A(u0,v0), D(u0,v1), C(u1,v1), B(u1,v0)
            // Triangle 1 : A, D, C
            // Triangle 2 : A, C, B
            auto emit = [&](float u, float v, float fu, float fv) {
                auto p = P(u,v); auto n = N(u,v);
                push3(m->positions, p[0], p[1], p[2]);
                push3(m->normals,   n[0], n[1], n[2]);
                push2(m->uvs, fu, fv);
            };

            float fu0=(float)i/rings, fu1=(float)(i+1)/rings;
            float fv0=(float)j/sectors, fv1=(float)(j+1)/sectors;

            emit(u0,v0,fu0,fv0); emit(u0,v1,fu0,fv1); emit(u1,v1,fu1,fv1);
            emit(u0,v0,fu0,fv0); emit(u1,v1,fu1,fv1); emit(u1,v0,fu1,fv0);
        }
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
