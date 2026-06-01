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
Mesh*    sphere       = nullptr;
Mesh*    cylinder     = nullptr;
Camera   cam;
Texture* tex_diffuse  = nullptr;

mygl::matrix4 g_projection;

// Uniforms cel
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
    glClearColor(0.85f, 0.85f, 0.9f, 1.0f); CHECK(); // fond clair
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

bool initshaders() {
    // Programme cel shading
    std::string cv = read_file("shaders/cel_vertex.glsl");
    std::string cf = read_file("shaders/cel_fragment.glsl");
    prog_cel = program::make_program(cv, cf);
    if (!prog_cel || !prog_cel->is_ready()) {
        std::cerr << "cel shader failed\n"; return false;
    }

    // Programme outline
    std::string ov = read_file("shaders/outline_vertex.glsl");
    std::string of = read_file("shaders/outline_fragment.glsl");
    prog_outline = program::make_program(ov, of);
    if (!prog_outline || !prog_outline->is_ready()) {
        std::cerr << "outline shader failed\n"; return false;
    }

    // Locations uniforms cel
    prog_cel->use();
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

    // Locations uniforms outline
    ul_mv_out   = glGetUniformLocation(prog_outline->id(), "model_view");
    ul_proj_out = glGetUniformLocation(prog_outline->id(), "projection");
    ul_width    = glGetUniformLocation(prog_outline->id(), "outline_width");
    ul_out_color= glGetUniformLocation(prog_outline->id(), "outline_color");

    std::cout << "Shaders OK\n";
    return true;
}

void initobjects() {
    sphere   = Mesh::make_sphere(48, 64, 1.0f);
    cylinder = Mesh::make_cylinder(64, 0.5f, 1.5f);
    sphere->upload();
    cylinder->upload();
    std::cout << "Sphere: " << sphere->vertex_count() << " vertices\n";
}

void inittextures() {
    // Texture optionnelle — fonctionne sans
    tex_diffuse = Texture::load("assets/textures/diffuse.tga");
}

void initPOV() {
    g_projection = mygl::frustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 100.0f);
}

// ─── rendu d'un objet (utilisé dans les 2 passes) ─────────────────────────
static void draw_object(Mesh* mesh, const mygl::matrix4& model,
                        GLint mv_loc, GLint proj_loc) {
    mygl::matrix4 mv = cam.get_view() * model;
    glUniformMatrix4fv(mv_loc,   1, GL_FALSE, mv.data());           CHECK();
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, g_projection.data()); CHECK();
    mesh->draw();
}

// ─── boucle de rendu ──────────────────────────────────────────────────────
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK();

    mygl::vec3 eye = cam.get_eye();

    // Scène : 2 objets
    mygl::matrix4 model_sphere   = mygl::matrix4::translate(-1.2f, 0, 0);
    mygl::matrix4 model_cylinder = mygl::matrix4::translate( 1.2f, 0, 0);

    // ── PASSE 1 : CONTOURS ────────────────────────────────────────────────
    glCullFace(GL_FRONT);     // on dessine les faces ARRIÈRE
    prog_outline->use();
    glUniform1f(ul_width,     0.04f);
    glUniform3f(ul_out_color, 0.0f, 0.0f, 0.0f);

    draw_object(sphere,   model_sphere,   ul_mv_out, ul_proj_out);
    draw_object(cylinder, model_cylinder, ul_mv_out, ul_proj_out);

    // ── PASSE 2 : CEL SHADING ─────────────────────────────────────────────
    glCullFace(GL_BACK);
    prog_cel->use();

    // Lumière fixe dans la scène
    glUniform3f(ul_light,    2.0f, 3.0f, 2.0f);
    glUniform3f(ul_lightcol, 1.0f, 1.0f, 1.0f);
    glUniform3f(ul_eye,      eye.x, eye.y, eye.z);

    // Paramètres cartoon
    glUniform1f(ul_bands,    4.0f);   // 4 paliers de couleur
    glUniform1f(ul_ambient,  0.15f);
    glUniform1f(ul_spec_thr, 0.5f);
    glUniform1f(ul_spec_str, 0.8f);
    glUniform1f(ul_rim_pow,  3.0f);
    glUniform1f(ul_rim_thr,  0.3f);
    glUniform3f(ul_rim_col,  0.3f, 0.5f, 1.0f); // rim bleuté

    // Sphère — couleur rouge-orangée
    glUniform1i(ul_use_tex,   0);
    glUniform4f(ul_obj_color, 0.9f, 0.3f, 0.1f, 1.0f);
    draw_object(sphere, model_sphere, ul_mv_cel, ul_proj_cel);

    // Cylindre — couleur verte
    glUniform4f(ul_obj_color, 0.2f, 0.7f, 0.3f, 1.0f);
    draw_object(cylinder, model_cylinder, ul_mv_cel, ul_proj_cel);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h); CHECK();
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
    glutKeyboardFunc([](unsigned char k, int, int){ cam.on_key(k); });
    glutSpecialFunc ([](int k, int, int)           { cam.on_special(k); });

    std::cout << "Controls: WASD = rotate, Q/E = zoom, ESC = quit\n";
    glutMainLoop();
}
