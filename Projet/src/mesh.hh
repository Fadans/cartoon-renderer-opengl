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
