#pragma once
#include <GL/glew.h>
#include <vector>

// Attribut locations fixes (layout(location=N) dans tous les shaders)
static const GLint ATTR_POSITION = 0;
static const GLint ATTR_NORMAL   = 1;
static const GLint ATTR_UV       = 2;

class Mesh {
public:
    ~Mesh();

    // Génération procédurale
    static Mesh* make_sphere(int rings, int sectors, float radius = 1.0f);
    // with_caps=true : mesh visuel complet
    // with_caps=false : flanc uniquement, pour le pass outline
    static Mesh* make_cylinder(int n, float radius = 0.5f, float height = 1.5f, bool with_caps = true);
    static Mesh* make_torus(int rings, int sectors, float R = 0.6f, float r = 0.25f);
    // Boîte centrée en l'origine, demi-dimensions (hx,hy,hz) — 6 faces, normales plates
    static Mesh* make_box(float hx, float hy, float hz);
    // Cône centré en l'origine : base (rayon `radius`) en y=-h/2, sommet en y=+h/2
    static Mesh* make_cone(int n, float radius, float height);
    // Rocher facetté : sphère basse résolution déformée par bruit, normales PLATES
    // (look low-poly anguleux). `jitter` = amplitude de déformation, `seed` = variante.
    static Mesh* make_rock(int rings, int sectors, float radius, float jitter, float seed);
    // Mesa / falaise : prisme à `sides` côtés, rayon érodé par colonne, sommet plat,
    // flancs à normale plate (look de butte désertique).
    static Mesh* make_mesa(int sides, float radius, float height, float jitter, float seed);
    // Butte : rocher facetté XXL tronqué (Y clampé) → sommet & base plats, flancs
    // rocheux bruités. `radius` = largeur, `height` = hauteur totale.
    static Mesh* make_butte(int rings, int sectors, float radius, float height, float jitter, float seed);
    // Touffe d'herbe : `blades` brins (triangles fins, double face) en éventail.
    static Mesh* make_grass(int blades, float height, float spread, float seed);
    // Quad horizontal (normale +Y) centré en (0,y,0), demi-côté `half`
    static Mesh* make_plane(float half, float y = 0.0f);

    // Upload vers le GPU (à appeler après initglew)
    void upload();

    // Dessin
    void draw() const;

    int vertex_count() const { return (int)positions.size() / 3; }

private:
    Mesh() = default;

    // Données CPU
    std::vector<GLfloat> positions;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> uvs;

    // GPU
    GLuint vao_id    = 0;
    GLuint vbo_pos   = 0;
    GLuint vbo_nor   = 0;
    GLuint vbo_uv    = 0;
    bool   uploaded_ = false;
};
