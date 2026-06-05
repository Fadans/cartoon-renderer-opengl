#include "mesh.hh"
#include <cmath>
#include <array>
#include <cstdio>

static const float PI = 3.14159265f;

Mesh::~Mesh() {
    if (vbo_pos) glDeleteBuffers(1, &vbo_pos);
    if (vbo_nor) glDeleteBuffers(1, &vbo_nor);
    if (vbo_uv)  glDeleteBuffers(1, &vbo_uv);
    if (vao_id)  glDeleteVertexArrays(1, &vao_id);
}

static void push3(std::vector<GLfloat>& v, float a, float b, float c) {
    v.push_back(a); v.push_back(b); v.push_back(c);
}
static void push2(std::vector<GLfloat>& v, float a, float b) {
    v.push_back(a); v.push_back(b);
}

static float rock_noise(float x, float y, float z, float seed) {
    float n = std::sin(x*2.7f + seed*1.3f)
            + std::sin(y*3.1f + seed*2.1f)
            + std::sin(z*2.3f + seed*0.7f)
            + std::sin((x+y)*1.9f + seed*0.5f)
            + std::sin((y+z)*2.5f + seed*1.7f);
    return n * 0.2f;
}

Mesh* Mesh::make_rock(int rings, int sectors, float radius, float jitter, float seed) {
    Mesh* m = new Mesh();
    auto dir = [](float phi, float th) -> std::array<float,3> {
        return { std::sin(phi)*std::cos(th), std::cos(phi), std::sin(phi)*std::sin(th) };
    };
    auto disp = [&](std::array<float,3> u) -> std::array<float,3> {
        float d = radius * (1.0f + jitter * rock_noise(u[0],u[1],u[2], seed));
        return { u[0]*d, u[1]*d, u[2]*d };
    };

    auto tri = [&](std::array<float,3> a, std::array<float,3> b, std::array<float,3> c) {
        float ux=b[0]-a[0], uy=b[1]-a[1], uz=b[2]-a[2];
        float vx=c[0]-a[0], vy=c[1]-a[1], vz=c[2]-a[2];
        float nx=uy*vz-uz*vy, ny=uz*vx-ux*vz, nz=ux*vy-uy*vx;
        float l=std::sqrt(nx*nx+ny*ny+nz*nz); if (l>1e-6f){ nx/=l; ny/=l; nz/=l; }
        auto P=[&](std::array<float,3> p){
            push3(m->positions,p[0],p[1],p[2]); push3(m->normals,nx,ny,nz); push2(m->uvs,0,0);
        };
        P(a); P(b); P(c);
    };
    for (int r = 0; r < rings; r++) {
        float phi0=PI*r/rings, phi1=PI*(r+1)/rings;
        for (int s = 0; s < sectors; s++) {
            float th0=2*PI*s/sectors, th1=2*PI*(s+1)/sectors;
            auto A=disp(dir(phi0,th0)), B=disp(dir(phi1,th0));
            auto C=disp(dir(phi1,th1)), D=disp(dir(phi0,th1));
            tri(A,D,C); tri(A,C,B);
        }
    }
    return m;
}

Mesh* Mesh::make_butte(int rings, int sectors, float radius, float height, float jitter, float seed) {
    Mesh* m = new Mesh();
    float hy = height * 0.5f;
    auto dir = [](float phi, float th) -> std::array<float,3> {
        return { std::sin(phi)*std::cos(th), std::cos(phi), std::sin(phi)*std::sin(th) };
    };

    auto pt = [&](std::array<float,3> u) -> std::array<float,3> {
        float fr = 1.0f + jitter * rock_noise(u[0],u[1],u[2], seed);
        if (fr < 0.35f) fr = 0.35f;
        float rr = radius * fr;
        float y  = u[1] * hy * 3.0f;
        y = y < -hy ? -hy : (y > hy ? hy : y);
        return { u[0]*rr, y, u[2]*rr };
    };
    auto tri = [&](std::array<float,3> a, std::array<float,3> b, std::array<float,3> c) {
        float ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2], vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
        float nx=uy*vz-uz*vy, ny=uz*vx-ux*vz, nz=ux*vy-uy*vx;
        float l=std::sqrt(nx*nx+ny*ny+nz*nz); if (l>1e-6f){ nx/=l; ny/=l; nz/=l; }
        auto P=[&](std::array<float,3> p){ push3(m->positions,p[0],p[1],p[2]); push3(m->normals,nx,ny,nz); push2(m->uvs,0,0); };
        P(a); P(b); P(c);
    };
    for (int r = 0; r < rings; r++) {
        float phi0=PI*r/rings, phi1=PI*(r+1)/rings;
        for (int s = 0; s < sectors; s++) {
            float th0=2*PI*s/sectors, th1=2*PI*(s+1)/sectors;
            auto A=pt(dir(phi0,th0)), B=pt(dir(phi1,th0)), C=pt(dir(phi1,th1)), D=pt(dir(phi0,th1));
            tri(A,D,C); tri(A,C,B);
        }
    }
    return m;
}

Mesh* Mesh::make_grass(int blades, float height, float spread, float seed) {
    Mesh* m = new Mesh();
    auto rnd=[&](float k){ float v=std::sin(seed*12.99f + k*78.23f)*43758.5f; return v-std::floor(v); };
    for(int i=0;i<blades;i++){
        float ang  = rnd(i*1.0f)*2*PI;
        float lean = 0.15f + rnd(i*1.7f)*0.45f;
        float h    = height*(0.6f + 0.6f*rnd(i*2.3f));
        float rad  = spread*rnd(i*3.1f);
        float bx=std::cos(ang)*rad, bz=std::sin(ang)*rad;
        float w=0.035f;
        float tx=std::cos(ang+1.5708f)*w, tz=std::sin(ang+1.5708f)*w;
        float px=bx+std::cos(ang)*lean*h, pz=bz+std::sin(ang)*lean*h;
        auto V=[&](float x,float y,float z){ push3(m->positions,x,y,z); push3(m->normals,0,1,0); push2(m->uvs,0,0); };
        V(bx-tx,0,bz-tz); V(bx+tx,0,bz+tz); V(px,h,pz);
        V(bx+tx,0,bz+tz); V(bx-tx,0,bz-tz); V(px,h,pz);
    }
    return m;
}

Mesh* Mesh::make_plane(float half, float y) {
    Mesh* m = new Mesh();

    auto v = [&](float x, float z, float u, float w) {
        push3(m->positions, x, y, z);
        push3(m->normals,   0, 1, 0);
        push2(m->uvs, u, w);
    };
    v(-half,-half,0,0); v(-half, half,0,1); v( half, half,1,1);
    v(-half,-half,0,0); v( half, half,1,1); v( half,-half,1,0);
    return m;
}

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

    for (auto& sub : submeshes) {
        if (!sub.tex_path.empty()) {
            sub.texture = Texture::load(sub.tex_path.c_str());
            if (!sub.texture)
                std::fprintf(stderr, "Texture manquante: %s\n", sub.tex_path.c_str());
        }
    }
    uploaded_ = true;
}

void Mesh::draw() const {
    glBindVertexArray(vao_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)positions.size() / 3);
    glBindVertexArray(0);
}

void Mesh::draw_textured(GLint ul_use_tex) const {
    glBindVertexArray(vao_id);
    for (const auto& sub : submeshes) {
        if (sub.texture && sub.texture->valid()) {
            glUniform1i(ul_use_tex, 1);
            sub.texture->bind(GL_TEXTURE0);
        } else {
            glUniform1i(ul_use_tex, 0);
        }
        glDrawArrays(GL_TRIANGLES, sub.start_vertex, sub.vertex_count);
    }
    glBindVertexArray(0);
}

void Mesh::push_vertex(float px, float py, float pz,
                       float nx, float ny, float nz, float u, float v) {
    push3(positions, px, py, pz);
    push3(normals,   nx, ny, nz);
    push2(uvs,       u,  v);
}

void Mesh::push_submesh(int start_vertex, int vertex_count, const std::string& tex_path) {
    SubMesh sub;
    sub.start_vertex = start_vertex;
    sub.vertex_count = vertex_count;
    sub.tex_path     = tex_path;
    submeshes.push_back(sub);
}
