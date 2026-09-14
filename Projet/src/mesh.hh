#pragma once
#include <GL/glew.h>
#include <vector>
#include <string>
#include "texture.hh"

static const GLint ATTR_POSITION = 0;
static const GLint ATTR_NORMAL   = 1;
static const GLint ATTR_UV       = 2;

struct SubMesh {
    int start_vertex;
    int vertex_count;
    std::string tex_path;
    Texture* texture = nullptr;
    float color[3] = { 0.7f, 0.7f, 0.7f };
};

class Mesh {
public:
    ~Mesh();

    static Mesh* make_sphere(int rings, int sectors, float radius = 1.0f);

    static Mesh* make_cylinder(int n, float radius = 0.5f, float height = 1.5f, bool with_caps = true);

    static Mesh* make_torus(int rings, int sectors, float R = 0.6f, float r = 0.25f);

    static Mesh* make_rock(int rings, int sectors, float radius, float jitter, float seed);

    static Mesh* make_butte(int rings, int sectors, float radius, float height, float jitter, float seed);

    static Mesh* make_grass(int blades, float height, float spread, float seed);

    static Mesh* make_plane(float half, float y = 0.0f);

    static Mesh* load_from_file(const std::string& path);

    void push_vertex(float px, float py, float pz,
                     float nx, float ny, float nz, float u, float v);
    void push_submesh(int start_vertex, int vertex_count, const std::string& tex_path,
                      float r, float g, float b);

    void upload();

    void draw() const;
    void draw_colored(GLint ul_use_tex, GLint ul_obj_color) const;

    void bounds(float mn[3], float mx[3]) const;

    int vertex_count() const { return (int)positions.size() / 3; }

private:
    Mesh() = default;

    std::vector<GLfloat> positions;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> uvs;
    std::vector<SubMesh> submeshes;

    GLuint vao_id    = 0;
    GLuint vbo_pos   = 0;
    GLuint vbo_nor   = 0;
    GLuint vbo_uv    = 0;
    bool   uploaded_ = false;
};
