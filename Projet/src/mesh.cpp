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

Mesh* Mesh::make_sphere(int rings, int sectors, float radius) {
    Mesh* m = new Mesh();
    for (int r = 0; r < rings; r++) {
        float phi0 = PI * r       / rings;
        float phi1 = PI * (r + 1) / rings;
        for (int s = 0; s < sectors; s++) {
            float theta0 = 2*PI * s       / sectors;
            float theta1 = 2*PI * (s + 1) / sectors;
            float x00=std::sin(phi0)*std::cos(theta0), y00=std::cos(phi0), z00=std::sin(phi0)*std::sin(theta0);
            float x10=std::sin(phi1)*std::cos(theta0), y10=std::cos(phi1), z10=std::sin(phi1)*std::sin(theta0);
            float x11=std::sin(phi1)*std::cos(theta1), y11=std::cos(phi1), z11=std::sin(phi1)*std::sin(theta1);
            float x01=std::sin(phi0)*std::cos(theta1), y01=std::cos(phi0), z01=std::sin(phi0)*std::sin(theta1);
            float u0=(float)s/sectors, u1=(float)(s+1)/sectors;
            float v0=(float)r/rings,   v1=(float)(r+1)/rings;
            push3(m->positions, x00*radius, y00*radius, z00*radius); push3(m->normals, x00, y00, z00); push2(m->uvs, u0, v0);
            push3(m->positions, x01*radius, y01*radius, z01*radius); push3(m->normals, x01, y01, z01); push2(m->uvs, u1, v0);
            push3(m->positions, x11*radius, y11*radius, z11*radius); push3(m->normals, x11, y11, z11); push2(m->uvs, u1, v1);
            push3(m->positions, x00*radius, y00*radius, z00*radius); push3(m->normals, x00, y00, z00); push2(m->uvs, u0, v0);
            push3(m->positions, x11*radius, y11*radius, z11*radius); push3(m->normals, x11, y11, z11); push2(m->uvs, u1, v1);
            push3(m->positions, x10*radius, y10*radius, z10*radius); push3(m->normals, x10, y10, z10); push2(m->uvs, u0, v1);
        }
    }
    return m;
}

Mesh* Mesh::make_cylinder(int n, float radius, float height, bool with_caps) {
    Mesh* m = new Mesh();
    for (int i = 0; i < n; i++) {
        float t0=2*PI*i/n, t1=2*PI*(i+1)/n;
        float x0=radius*std::cos(t0), z0=radius*std::sin(t0);
        float x1=radius*std::cos(t1), z1=radius*std::sin(t1);
        float u0=(float)i/n, u1=(float)(i+1)/n;
        push3(m->positions,x0,-height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,0);
        push3(m->positions,x0, height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,1);
        push3(m->positions,x1, height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,1);
        push3(m->positions,x0,-height/2,z0); push3(m->normals,x0/radius,0,z0/radius); push2(m->uvs,u0,0);
        push3(m->positions,x1, height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,1);
        push3(m->positions,x1,-height/2,z1); push3(m->normals,x1/radius,0,z1/radius); push2(m->uvs,u1,0);
        if (with_caps) {
            float cu0=0.5f+0.5f*std::cos(t0), cv0=0.5f+0.5f*std::sin(t0);
            float cu1=0.5f+0.5f*std::cos(t1), cv1=0.5f+0.5f*std::sin(t1);
            push3(m->positions,0, height/2, 0); push3(m->normals,0,1,0); push2(m->uvs,.5f,.5f);
            push3(m->positions,x1,height/2,z1); push3(m->normals,0,1,0); push2(m->uvs,cu1,cv1);
            push3(m->positions,x0,height/2,z0); push3(m->normals,0,1,0); push2(m->uvs,cu0,cv0);
            push3(m->positions,0,-height/2, 0); push3(m->normals,0,-1,0); push2(m->uvs,.5f,.5f);
            push3(m->positions,x0,-height/2,z0); push3(m->normals,0,-1,0); push2(m->uvs,cu0,cv0);
            push3(m->positions,x1,-height/2,z1); push3(m->normals,0,-1,0); push2(m->uvs,cu1,cv1);
        }
    }
    return m;
}

Mesh* Mesh::make_torus(int rings, int sectors, float R, float r) {
    Mesh* m = new Mesh();
    for (int i = 0; i < rings; i++) {
        float u0 = 2*PI * i / rings, u1 = 2*PI * (i + 1) / rings;
        for (int j = 0; j < sectors; j++) {
            float v0 = 2*PI * j / sectors, v1 = 2*PI * (j + 1) / sectors;
            auto P = [&](float u, float v) -> std::array<float,3> {
                return { (R + r*std::cos(v))*std::cos(u), r*std::sin(v), (R + r*std::cos(v))*std::sin(u) };
            };
            auto N = [](float u, float v) -> std::array<float,3> {
                return { std::cos(v)*std::cos(u), std::sin(v), std::cos(v)*std::sin(u) };
            };
            auto emit = [&](float u, float v, float fu, float fv) {
                auto p = P(u,v); auto n = N(u,v);
                push3(m->positions, p[0], p[1], p[2]); push3(m->normals, n[0], n[1], n[2]); push2(m->uvs, fu, fv);
            };
            float fu0=(float)i/rings, fu1=(float)(i+1)/rings;
            float fv0=(float)j/sectors, fv1=(float)(j+1)/sectors;
            emit(u0,v0,fu0,fv0); emit(u0,v1,fu0,fv1); emit(u1,v1,fu1,fv1);
            emit(u0,v0,fu0,fv0); emit(u1,v1,fu1,fv1); emit(u1,v0,fu1,fv0);
        }
    }
    return m;
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

void Mesh::push_vertex(float px, float py, float pz,
                       float nx, float ny, float nz, float u, float v) {
    push3(positions, px, py, pz);
    push3(normals,   nx, ny, nz);
    push2(uvs,       u,  v);
}

void Mesh::push_submesh(int start_vertex, int vertex_count, const std::string& tex_path,
                        float r, float g, float b) {
    SubMesh sub;
    sub.start_vertex = start_vertex;
    sub.vertex_count = vertex_count;
    sub.tex_path     = tex_path;
    sub.color[0] = r; sub.color[1] = g; sub.color[2] = b;
    submeshes.push_back(sub);
}

void Mesh::draw_colored(GLint ul_use_tex, GLint ul_obj_color) const {
    glBindVertexArray(vao_id);
    for (const auto& sub : submeshes) {
        if (sub.texture && sub.texture->valid()) {
            glUniform1i(ul_use_tex, 1);
            sub.texture->bind(GL_TEXTURE0);
        } else {
            glUniform1i(ul_use_tex, 0);
            glUniform4f(ul_obj_color, sub.color[0], sub.color[1], sub.color[2], 1.0f);
        }
        glDrawArrays(GL_TRIANGLES, sub.start_vertex, sub.vertex_count);
    }
    glBindVertexArray(0);
}

void Mesh::bounds(float mn[3], float mx[3]) const {
    if (positions.empty()) { mn[0]=mn[1]=mn[2]=0; mx[0]=mx[1]=mx[2]=0; return; }
    mn[0]=mx[0]=positions[0]; mn[1]=mx[1]=positions[1]; mn[2]=mx[2]=positions[2];
    for (size_t i = 0; i + 2 < positions.size(); i += 3)
        for (int k = 0; k < 3; k++) {
            float c = positions[i+k];
            if (c < mn[k]) mn[k] = c;
            if (c > mx[k]) mx[k] = c;
        }
}
