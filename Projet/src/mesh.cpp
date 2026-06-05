#include "mesh.hh"
#include <cmath>
#include <array>
#include <cstdio>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

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

// ─── boîte (cube paramétrable) ──────────────────────────────────────────────
// 6 faces planes, normale constante par face (look facetté = cartoon).
// Pour chaque face : centre + 2 demi-arêtes (u,v) tels que u×v = normale sortante.
Mesh* Mesh::make_box(float hx, float hy, float hz) {
    Mesh* m = new Mesh();
    auto face = [&](float cx,float cy,float cz,
                    float ux,float uy,float uz,
                    float vx,float vy,float vz,
                    float nx,float ny,float nz) {
        float c0[3]={cx-ux-vx, cy-uy-vy, cz-uz-vz};
        float c1[3]={cx+ux-vx, cy+uy-vy, cz+uz-vz};
        float c2[3]={cx+ux+vx, cy+uy+vy, cz+uz+vz};
        float c3[3]={cx-ux+vx, cy-uy+vy, cz-uz+vz};
        auto vert=[&](float* c,float u,float w){
            push3(m->positions,c[0],c[1],c[2]); push3(m->normals,nx,ny,nz); push2(m->uvs,u,w);
        };
        vert(c0,0,0); vert(c1,1,0); vert(c2,1,1);   // CCW vue de l'extérieur
        vert(c0,0,0); vert(c2,1,1); vert(c3,0,1);
    };
    face( hx,0,0,  0,hy,0,  0,0,hz,   1, 0, 0);   // +X
    face(-hx,0,0,  0,0,hz,  0,hy,0,  -1, 0, 0);   // -X
    face(0, hy,0,  0,0,hz,  hx,0,0,   0, 1, 0);   // +Y
    face(0,-hy,0,  hx,0,0,  0,0,hz,   0,-1, 0);   // -Y
    face(0,0, hz,  hx,0,0,  0,hy,0,   0, 0, 1);   // +Z
    face(0,0,-hz,  0,hy,0,  hx,0,0,   0, 0,-1);   // -Z
    return m;
}

// ─── cône (flanc + cap bas) ─────────────────────────────────────────────────
// Base de rayon `radius` en y=-h/2, sommet en y=+h/2.
// Normale du flanc = normalize(cos θ·h, radius, sin θ·h) : pente ∝ radius/h.
Mesh* Mesh::make_cone(int n, float radius, float height) {
    Mesh* m = new Mesh();
    float hy = height / 2.0f;
    auto sideN = [&](float th) -> std::array<float,3> {
        float nx=std::cos(th)*height, ny=radius, nz=std::sin(th)*height;
        float l=std::sqrt(nx*nx+ny*ny+nz*nz);
        return { nx/l, ny/l, nz/l };
    };
    for (int i = 0; i < n; i++) {
        float t0=2*PI*i/n, t1=2*PI*(i+1)/n, tm=2*PI*(i+0.5f)/n;
        float x0=radius*std::cos(t0), z0=radius*std::sin(t0);
        float x1=radius*std::cos(t1), z1=radius*std::sin(t1);
        auto n0=sideN(t0), n1=sideN(t1), nm=sideN(tm);
        float u0=(float)i/n, u1=(float)(i+1)/n;

        // Flanc — base0, sommet, base1 (CCW vu de dehors)
        push3(m->positions,x0,-hy,z0); push3(m->normals,n0[0],n0[1],n0[2]); push2(m->uvs,u0,0);
        push3(m->positions,0,  hy, 0); push3(m->normals,nm[0],nm[1],nm[2]); push2(m->uvs,(u0+u1)*0.5f,1);
        push3(m->positions,x1,-hy,z1); push3(m->normals,n1[0],n1[1],n1[2]); push2(m->uvs,u1,0);

        // Cap bas (normale -Y) — center, base0, base1
        float cu0=0.5f+0.5f*std::cos(t0), cv0=0.5f+0.5f*std::sin(t0);
        float cu1=0.5f+0.5f*std::cos(t1), cv1=0.5f+0.5f*std::sin(t1);
        push3(m->positions,0,-hy,0);  push3(m->normals,0,-1,0); push2(m->uvs,.5f,.5f);
        push3(m->positions,x0,-hy,z0);push3(m->normals,0,-1,0); push2(m->uvs,cu0,cv0);
        push3(m->positions,x1,-hy,z1);push3(m->normals,0,-1,0); push2(m->uvs,cu1,cv1);
    }
    return m;
}

// ─── rocher facetté (sphère déformée par bruit, normales plates) ─────────────
// Bruit lisse déterministe (lumps basse fréquence) à partir d'une direction.
// Déterministe sur la DIRECTION → les sommets dupliqués entre triangles reçoivent
// le même déplacement (pas de fissure dans le maillage).
static float rock_noise(float x, float y, float z, float seed) {
    float n = std::sin(x*2.7f + seed*1.3f)
            + std::sin(y*3.1f + seed*2.1f)
            + std::sin(z*2.3f + seed*0.7f)
            + std::sin((x+y)*1.9f + seed*0.5f)
            + std::sin((y+z)*2.5f + seed*1.7f);
    return n * 0.2f;   // ≈ [-1,1]
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
    // Triangle à normale PLATE (calculée depuis les 3 sommets déformés → facettes)
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
            tri(A,D,C); tri(A,C,B);   // même winding CCW que make_sphere
        }
    }
    return m;
}

// ─── butte (rocher facetté tronqué : sommet & base plats) ────────────────────
Mesh* Mesh::make_butte(int rings, int sectors, float radius, float height, float jitter, float seed) {
    Mesh* m = new Mesh();
    float hy = height * 0.5f;
    auto dir = [](float phi, float th) -> std::array<float,3> {
        return { std::sin(phi)*std::cos(th), std::cos(phi), std::sin(phi)*std::sin(th) };
    };
    // Point : rayon horizontal bruité (flancs rocheux) ; Y étiré puis CLAMPÉ → plat haut/bas.
    auto pt = [&](std::array<float,3> u) -> std::array<float,3> {
        float fr = 1.0f + jitter * rock_noise(u[0],u[1],u[2], seed);
        if (fr < 0.35f) fr = 0.35f;                   // garde un rayon positif même à fort jitter
        float rr = radius * fr;
        float y  = u[1] * hy * 3.0f;                  // étire très fort → grand plateau plat (troncature +++)
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

// ─── mesa / falaise (prisme érodé, sommet plat) ──────────────────────────────
Mesh* Mesh::make_mesa(int sides, float radius, float height, float jitter, float seed) {
    Mesh* m = new Mesh();
    float hy = height / 2.0f;
    // rayon par colonne : modulé par 2 sinus → silhouette érodée, irrégulière
    auto rad = [&](int i) {
        float a = 2.0f*PI*i/sides;
        return radius * (1.0f + jitter*0.5f*(std::sin(a*2.0f + seed) + std::sin(a*3.0f + seed*1.7f)));
    };
    auto tri = [&](float ax,float ay,float az, float bx,float by,float bz, float cx,float cy,float cz) {
        float ux=bx-ax,uy=by-ay,uz=bz-az, vx=cx-ax,vy=cy-ay,vz=cz-az;
        float nx=uy*vz-uz*vy, ny=uz*vx-ux*vz, nz=ux*vy-uy*vx;
        float l=std::sqrt(nx*nx+ny*ny+nz*nz); if (l>1e-6f){ nx/=l; ny/=l; nz/=l; }
        push3(m->positions,ax,ay,az); push3(m->normals,nx,ny,nz); push2(m->uvs,0,0);
        push3(m->positions,bx,by,bz); push3(m->normals,nx,ny,nz); push2(m->uvs,0,0);
        push3(m->positions,cx,cy,cz); push3(m->normals,nx,ny,nz); push2(m->uvs,0,0);
    };
    for (int i = 0; i < sides; i++) {
        float t0=2*PI*i/sides, t1=2*PI*(i+1)/sides;
        float r0=rad(i), r1=rad((i+1)%sides);
        float x0=r0*std::cos(t0), z0=r0*std::sin(t0);
        float x1=r1*std::cos(t1), z1=r1*std::sin(t1);
        // Flanc (2 triangles) — même winding CCW que le cylindre
        tri(x0,-hy,z0,  x0,hy,z0,  x1,hy,z1);
        tri(x0,-hy,z0,  x1,hy,z1,  x1,-hy,z1);
        // Sommet plat (normale +Y) : centre, haut1, haut0 (CCW vu de dessus)
        tri(0,hy,0,  x1,hy,z1,  x0,hy,z0);
    }
    return m;
}

// ─── touffe d'herbe (brins fins, double face) ────────────────────────────────
Mesh* Mesh::make_grass(int blades, float height, float spread, float seed) {
    Mesh* m = new Mesh();
    auto rnd=[&](float k){ float v=std::sin(seed*12.99f + k*78.23f)*43758.5f; return v-std::floor(v); };
    for(int i=0;i<blades;i++){
        float ang  = rnd(i*1.0f)*2*PI;
        float lean = 0.15f + rnd(i*1.7f)*0.45f;            // inclinaison du brin
        float h    = height*(0.6f + 0.6f*rnd(i*2.3f));
        float rad  = spread*rnd(i*3.1f);
        float bx=std::cos(ang)*rad, bz=std::sin(ang)*rad;  // base décalée du centre
        float w=0.035f;                                    // demi-largeur à la base
        float tx=std::cos(ang+1.5708f)*w, tz=std::sin(ang+1.5708f)*w;
        float px=bx+std::cos(ang)*lean*h, pz=bz+std::sin(ang)*lean*h; // pointe (penchée)
        auto V=[&](float x,float y,float z){ push3(m->positions,x,y,z); push3(m->normals,0,1,0); push2(m->uvs,0,0); };
        V(bx-tx,0,bz-tz); V(bx+tx,0,bz+tz); V(px,h,pz);    // face avant
        V(bx+tx,0,bz+tz); V(bx-tx,0,bz-tz); V(px,h,pz);    // face arrière (double face)
    }
    return m;
}

// ─── sol (quad horizontal) ──────────────────────────────────────────────────
Mesh* Mesh::make_plane(float half, float y) {
    Mesh* m = new Mesh();
    // 2 triangles, normale +Y, CCW vu de dessus
    auto v = [&](float x, float z, float u, float w) {
        push3(m->positions, x, y, z);
        push3(m->normals,   0, 1, 0);
        push2(m->uvs, u, w);
    };
    v(-half,-half,0,0); v(-half, half,0,1); v( half, half,1,1);
    v(-half,-half,0,0); v( half, half,1,1); v( half,-half,1,0);
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

    // Modèles chargés (Assimp) : charger la texture de chaque sous-mesh (sinon
    // `submeshes` est vide pour les meshes procéduraux → boucle sans effet).
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

// Dessine en liant la texture de chaque sous-mesh (modèles Assimp).
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

// ─── chargement de modèle externe via Assimp ─────────────────────────────────
void Mesh::process_node(aiNode* node, const aiScene* scene,
                        Mesh* m, aiMatrix4x4 parent_transform,
                        const std::string& dir) {
    aiMatrix4x4 transform = parent_transform * node->mTransformation;

    for (unsigned int mi = 0; mi < node->mNumMeshes; mi++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[mi]];
        SubMesh sub;
        sub.start_vertex = (int)m->positions.size() / 3;

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int idx = face.mIndices[j];
                aiVector3D pos = transform * mesh->mVertices[idx];
                push3(m->positions, pos.x, pos.y, pos.z);
                if (mesh->HasNormals()) {
                    aiMatrix3x3 normalMat(transform); normalMat.Transpose();
                    aiVector3D nor = normalMat * mesh->mNormals[idx]; nor.Normalize();
                    push3(m->normals, nor.x, nor.y, nor.z);
                } else {
                    push3(m->normals, 0, 1, 0);
                }
                if (mesh->mTextureCoords[0])
                    push2(m->uvs, mesh->mTextureCoords[0][idx].x, mesh->mTextureCoords[0][idx].y);
                else
                    push2(m->uvs, 0, 0);
            }
        }
        sub.vertex_count = (int)m->positions.size() / 3 - sub.start_vertex;

        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiString tex_name;
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex_name) == AI_SUCCESS)
                sub.tex_path = dir + tex_name.C_Str();
        }
        m->submeshes.push_back(sub);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
        process_node(node->mChildren[i], scene, m, transform, dir);
}

Mesh* Mesh::load_from_file(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene || !scene->mRootNode) {
        std::fprintf(stderr, "Assimp: impossible de charger %s\n", path.c_str());
        return nullptr;
    }
    std::string dir = path.substr(0, path.find_last_of("/\\") + 1);
    Mesh* m = new Mesh();
    aiMatrix4x4 identity;
    process_node(scene->mRootNode, scene, m, identity, dir);
    std::printf("Loaded %s : %d verts, %d submeshes\n",
                path.c_str(), (int)m->positions.size() / 3, (int)m->submeshes.size());
    return m;
}
