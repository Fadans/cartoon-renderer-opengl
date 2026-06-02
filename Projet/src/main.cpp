#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "matrix4.hh"
#include "program.hh"
#include "mesh.hh"
#include "camera.hh"
#include "texture.hh"

#define CHECK()                                                             \
  do { GLenum e=glGetError();                                              \
    if(e!=GL_NO_ERROR) std::cerr<<"GL error "<<e<<" line "<<__LINE__<<"\n";\
  } while(0)

// ─── état global ──────────────────────────────────────────────────────────
program* prog_cel     = nullptr;
program* prog_outline = nullptr;

Mesh*    sphere            = nullptr;
Mesh*    cylinder          = nullptr;  // visuel (avec caps)
Mesh*    cylinder_outline  = nullptr;  // outline (flanc uniquement, sans caps)
Mesh*    torus             = nullptr;

Camera   cam;

Texture* tex_sphere   = nullptr;  // checkerboard orange/blanc
Texture* tex_cylinder = nullptr;  // checkerboard vert/blanc
Texture* tex_torus    = nullptr;  // checkerboard bleu/blanc

mygl::matrix4 g_projection;

// Animation
float g_time      = 0.0f;
bool  g_animating = true;

// Paramètre cartoon interactif
float g_num_bands = 4.0f;  // toggle +/-

// Uniforms cel
GLint ul_model_cel;  // matrice model seule (espace monde pour éclairage)
GLint ul_mv_cel, ul_proj_cel, ul_light, ul_lightcol, ul_eye;
GLint ul_bands, ul_ambient, ul_spec_thr, ul_spec_str;
GLint ul_rim_pow, ul_rim_thr, ul_rim_col;
GLint ul_use_tex, ul_obj_color, ul_tex_diff;

// Uniforms outline
GLint ul_mv_out, ul_proj_out, ul_width, ul_out_color;

// ─── helper lire fichier ──────────────────────────────────────────────────
static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::cerr << "Cannot open: " << path << "\n"; return ""; }
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

// ─── initialisations ──────────────────────────────────────────────────────

bool initglut(int& argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitContextVersion(4, 5);
    glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(10, 10);
    glutCreateWindow("Rendu Cartoon — POGL");
    return true;
}

bool initglew() {
    if (glewInit() != GLEW_OK) { std::cerr << "glewInit failed\n"; return false; }
    return true;
}

void initGL() {
    glEnable(GL_DEPTH_TEST);  CHECK();
    glEnable(GL_CULL_FACE);   CHECK();
    glCullFace(GL_BACK);      CHECK();
    glFrontFace(GL_CCW);      CHECK();
    glClearColor(0.85f, 0.85f, 0.9f, 1.0f); CHECK();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

bool initshaders() {
    prog_cel = program::make_program(
        read_file("shaders/cel_vertex.glsl"),
        read_file("shaders/cel_fragment.glsl"));
    if (!prog_cel || !prog_cel->is_ready()) {
        std::cerr << "cel shader failed\n"; return false;
    }
    prog_outline = program::make_program(
        read_file("shaders/outline_vertex.glsl"),
        read_file("shaders/outline_fragment.glsl"));
    if (!prog_outline || !prog_outline->is_ready()) {
        std::cerr << "outline shader failed\n"; return false;
    }

    prog_cel->use();
    ul_model_cel = glGetUniformLocation(prog_cel->id(), "model");
    ul_mv_cel    = glGetUniformLocation(prog_cel->id(), "model_view");
    ul_proj_cel  = glGetUniformLocation(prog_cel->id(), "projection");
    ul_light     = glGetUniformLocation(prog_cel->id(), "light_position");
    ul_lightcol  = glGetUniformLocation(prog_cel->id(), "light_color");
    ul_eye       = glGetUniformLocation(prog_cel->id(), "eye_position");
    ul_bands     = glGetUniformLocation(prog_cel->id(), "num_bands");
    ul_ambient   = glGetUniformLocation(prog_cel->id(), "ambient");
    ul_spec_thr  = glGetUniformLocation(prog_cel->id(), "spec_threshold");
    ul_spec_str  = glGetUniformLocation(prog_cel->id(), "spec_strength");
    ul_rim_pow   = glGetUniformLocation(prog_cel->id(), "rim_power");
    ul_rim_thr   = glGetUniformLocation(prog_cel->id(), "rim_threshold");
    ul_rim_col   = glGetUniformLocation(prog_cel->id(), "rim_color");
    ul_use_tex   = glGetUniformLocation(prog_cel->id(), "use_texture");
    ul_obj_color = glGetUniformLocation(prog_cel->id(), "object_color");
    ul_tex_diff  = glGetUniformLocation(prog_cel->id(), "tex_diffuse");

    ul_mv_out    = glGetUniformLocation(prog_outline->id(), "model_view");
    ul_proj_out  = glGetUniformLocation(prog_outline->id(), "projection");
    ul_width     = glGetUniformLocation(prog_outline->id(), "outline_width");
    ul_out_color = glGetUniformLocation(prog_outline->id(), "outline_color");

    std::cout << "Shaders OK\n";
    return true;
}

void initobjects() {
    sphere           = Mesh::make_sphere(48, 64, 1.0f);
    cylinder         = Mesh::make_cylinder(64, 0.5f, 1.5f, true);   // caps pour le rendu
    cylinder_outline = Mesh::make_cylinder(64, 0.5f, 1.5f, false);  // pas de caps pour l'outline
    torus            = Mesh::make_torus(48, 64, 0.65f, 0.28f);
    sphere->upload();
    cylinder->upload();
    cylinder_outline->upload();
    torus->upload();
    std::cout << "Sphere: "   << sphere->vertex_count()   << " verts\n";
    std::cout << "Cylinder: " << cylinder->vertex_count() << " verts\n";
    std::cout << "Torus: "    << torus->vertex_count()    << " verts\n";
}

void inittextures() {
    // Checkerboards procéduraux — aucun fichier TGA nécessaire
    // Sphère : orange et crème
    tex_sphere   = Texture::checkerboard(128, 8,
                       230, 100,  30,   // orange
                       240, 220, 180);  // crème
    // Cylindre : vert sapin et vert clair
    tex_cylinder = Texture::checkerboard(128, 8,
                        30, 100,  50,   // vert foncé
                       180, 230, 180);  // vert clair
    // Tore : bleu nuit et bleu ciel
    tex_torus    = Texture::checkerboard(128, 8,
                        30,  60, 160,   // bleu nuit
                       150, 190, 240);  // bleu ciel

    // Lier le sampler tex_diffuse à l'unité 0
    prog_cel->use();
    glUniform1i(ul_tex_diff, 0);
}

void initPOV() {
    g_projection = mygl::frustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 100.0f);
}

// ─── timer d'animation ────────────────────────────────────────────────────
void timer(int) {
    if (g_animating) {
        g_time += 0.016f;   // ~60 fps
        glutPostRedisplay();
    }
    glutTimerFunc(16, timer, 0);
}

// ─── rendu d'un objet ─────────────────────────────────────────────────────
static void draw_cel(Mesh* mesh, const mygl::matrix4& model) {
    mygl::matrix4 mv = cam.get_view() * model;
    glUniformMatrix4fv(ul_model_cel, 1, GL_FALSE, model.data()); // espace monde
    glUniformMatrix4fv(ul_mv_cel,    1, GL_FALSE, mv.data());
    glUniformMatrix4fv(ul_proj_cel,  1, GL_FALSE, g_projection.data());
    mesh->draw();
}

static void draw_outline(Mesh* mesh, const mygl::matrix4& model) {
    mygl::matrix4 mv = cam.get_view() * model;
    glUniformMatrix4fv(ul_mv_out,   1, GL_FALSE, mv.data());
    glUniformMatrix4fv(ul_proj_out, 1, GL_FALSE, g_projection.data());
    mesh->draw();
}

// ─── boucle de rendu ──────────────────────────────────────────────────────
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK();

    mygl::vec3 eye = cam.get_eye();

    // Matrices model avec rotation temporelle — vitesses différentes par objet
    mygl::matrix4 model_sphere   = mygl::matrix4::translate(-2.5f, 0, 0)
                                 * mygl::matrix4::rotate_y(g_time * 0.7f);
    mygl::matrix4 model_cylinder = mygl::matrix4::translate( 0.0f, 0, 0)
                                 * mygl::matrix4::rotate_y(g_time * 1.0f);
    mygl::matrix4 model_torus    = mygl::matrix4::translate( 2.5f, 0, 0)
                                 * mygl::matrix4::rotate_y(g_time * 0.5f)
                                 * mygl::matrix4::rotate_x(g_time * 0.3f);

    // ── PASSE 1 : CONTOURS ────────────────────────────────────────────────
    glCullFace(GL_FRONT);
    prog_outline->use();
    glUniform1f(ul_width,     0.04f);
    glUniform3f(ul_out_color, 0.0f, 0.0f, 0.0f);
    draw_outline(sphere,           model_sphere);
    draw_outline(cylinder_outline, model_cylinder);  // flanc uniquement → plus d'ovale flottant
    draw_outline(torus,            model_torus);

    // ── PASSE 2 : CEL SHADING ─────────────────────────────────────────────
    glCullFace(GL_BACK);
    prog_cel->use();

    glUniform3f(ul_light,    2.0f, 4.0f, 3.0f);  // lumière monde fixe
    glUniform3f(ul_lightcol, 1.0f, 1.0f, 1.0f);
    glUniform3f(ul_eye,      eye.x, eye.y, eye.z);

    glUniform1f(ul_bands,    g_num_bands);
    glUniform1f(ul_ambient,  0.15f);
    glUniform1f(ul_spec_thr, 0.55f);
    glUniform1f(ul_spec_str, 0.7f);
    glUniform1f(ul_rim_pow,  4.0f);   // halo plus fin
    glUniform1f(ul_rim_thr,  0.55f);  // seuil plus haut → bande plus étroite
    glUniform3f(ul_rim_col,  0.7f, 0.85f, 1.0f);  // bleu pâle, moins agressif

    // Sphère — texture checkerboard orange
    glUniform1i(ul_use_tex, 1);
    tex_sphere->bind(GL_TEXTURE0);
    draw_cel(sphere, model_sphere);

    // Cylindre — texture checkerboard vert
    tex_cylinder->bind(GL_TEXTURE0);
    draw_cel(cylinder, model_cylinder);

    // Tore — texture checkerboard bleu
    tex_torus->bind(GL_TEXTURE0);
    draw_cel(torus, model_torus);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h); CHECK();
}

// ─── clavier ──────────────────────────────────────────────────────────────
void keyboard(unsigned char k, int, int) {
    switch (k) {
        // Nombre de bandes cel (démo interactive)
        case '+': case '=':
            g_num_bands = std::min(g_num_bands + 1.0f, 16.0f);
            std::cout << "num_bands = " << g_num_bands << "\n";
            glutPostRedisplay(); break;
        case '-': case '_':
            g_num_bands = std::max(g_num_bands - 1.0f, 1.0f);
            std::cout << "num_bands = " << g_num_bands << "\n";
            glutPostRedisplay(); break;
        // Pause animation
        case ' ':
            g_animating = !g_animating;
            std::cout << (g_animating ? "Animation ON\n" : "Animation OFF\n"); break;
        default:
            cam.on_key(k);
    }
}

// ─── main ─────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (!initglut(argc, argv)) return 1;
    if (!initglew())           return 1;
    initGL();
    if (!initshaders())        return 1;
    initobjects();
    inittextures();
    initPOV();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc([](int k, int, int){ cam.on_special(k); });
    glutTimerFunc(16, timer, 0);

    std::cout << "WASD/fleches=orbite  Q/E=zoom  +/-=bandes  ESPACE=pause  ESC=quitter\n";
    glutMainLoop();
}
