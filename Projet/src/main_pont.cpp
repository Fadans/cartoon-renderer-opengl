#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "matrix4.hh"
#include "program.hh"
#include "mesh.hh"
#include "camera.hh"
#include "style.hh"

#define CHECK()                                                             \
  do { GLenum e=glGetError();                                              \
    if(e!=GL_NO_ERROR) std::cerr<<"GL error "<<e<<" line "<<__LINE__<<"\n";\
  } while(0)

program* prog_cel  = nullptr;
program* prog_post = nullptr;

Mesh*    model_mesh = nullptr;
mygl::matrix4 g_fit;

Camera   cam;

GLuint g_fbo        = 0;
GLuint g_tex_color  = 0;
GLuint g_tex_normal = 0;
GLuint g_tex_depth  = 0;
GLuint g_quad_vao   = 0;
int    g_win_w      = 1024;
int    g_win_h      = 1024;

mygl::matrix4 g_projection;

float g_time      = 0.0f;
bool  g_animating = true;
bool  g_spin      = true;
float g_num_bands = 4.0f;
bool  g_hatch     = false;
int   g_style     = 0;

static void apply_style(int i) {
    g_style = i;
    g_num_bands = g_styles[i].num_bands;
    g_hatch = g_styles[i].hatch;
    std::cout << "Style " << (i + 1) << " : " << g_styles[i].name << "\n";
    glutPostRedisplay();
}

GLint ul_model_cel;
GLint ul_mv_cel, ul_proj_cel, ul_light, ul_lightcol, ul_eye;
GLint ul_bands, ul_ambient, ul_spec_thr, ul_spec_str;
GLint ul_use_tex, ul_obj_color, ul_use_hatch;

GLint ul_post_texel, ul_post_color, ul_post_depth_thr, ul_post_normal_thr, ul_post_thickness;
GLint ul_post_color_far, ul_post_thickness_far, ul_post_dist_near, ul_post_dist_far;
GLint ul_post_bg_top, ul_post_bg_bottom;

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::cerr << "Cannot open: " << path << "\n"; return ""; }
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

bool initglut(int& argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitContextVersion(4, 5);
    glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(10, 10);
    glutCreateWindow("Scene pont japonais (glTF) — Cel + contour");
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
    if (!prog_cel || !prog_cel->is_ready()) { std::cerr << "cel shader failed\n"; return false; }
    prog_post = program::make_program(
        read_file("shaders/post_vertex.glsl"),
        read_file("shaders/post_fragment.glsl"));
    if (!prog_post || !prog_post->is_ready()) { std::cerr << "post shader failed\n"; return false; }

    prog_cel->use();
    ul_model_cel = glGetUniformLocation(prog_cel->id(), "model");
    ul_mv_cel    = glGetUniformLocation(prog_cel->id(), "model_view");
    ul_proj_cel  = glGetUniformLocation(prog_cel->id(), "projection");
    ul_light     = glGetUniformLocation(prog_cel->id(), "light_dir");
    ul_lightcol  = glGetUniformLocation(prog_cel->id(), "light_color");
    ul_eye       = glGetUniformLocation(prog_cel->id(), "eye_position");
    ul_bands     = glGetUniformLocation(prog_cel->id(), "num_bands");
    ul_ambient   = glGetUniformLocation(prog_cel->id(), "ambient");
    ul_spec_thr  = glGetUniformLocation(prog_cel->id(), "spec_threshold");
    ul_spec_str  = glGetUniformLocation(prog_cel->id(), "spec_strength");
    ul_use_tex   = glGetUniformLocation(prog_cel->id(), "use_texture");
    ul_obj_color = glGetUniformLocation(prog_cel->id(), "object_color");
    ul_use_hatch = glGetUniformLocation(prog_cel->id(), "use_hatch");
    glUniform1i(glGetUniformLocation(prog_cel->id(), "tex_diffuse"), 0);
    glUniform1f(glGetUniformLocation(prog_cel->id(), "hatch_thr"), 0.6f);
    glUniform1i(glGetUniformLocation(prog_cel->id(), "two_sided"), 1);

    prog_post->use();
    glUniform1i(glGetUniformLocation(prog_post->id(), "u_color"),  0);
    glUniform1i(glGetUniformLocation(prog_post->id(), "u_normal"), 1);
    glUniform1i(glGetUniformLocation(prog_post->id(), "u_depth"),  2);
    ul_post_texel      = glGetUniformLocation(prog_post->id(), "u_texel");
    ul_post_color      = glGetUniformLocation(prog_post->id(), "u_outline_color");
    ul_post_depth_thr  = glGetUniformLocation(prog_post->id(), "u_depth_thr");
    ul_post_normal_thr = glGetUniformLocation(prog_post->id(), "u_normal_thr");
    ul_post_thickness  = glGetUniformLocation(prog_post->id(), "u_thickness");
    ul_post_color_far     = glGetUniformLocation(prog_post->id(), "u_outline_color_far");
    ul_post_thickness_far = glGetUniformLocation(prog_post->id(), "u_thickness_far");
    ul_post_dist_near     = glGetUniformLocation(prog_post->id(), "u_dist_near");
    ul_post_dist_far      = glGetUniformLocation(prog_post->id(), "u_dist_far");
    ul_post_bg_top        = glGetUniformLocation(prog_post->id(), "u_bg_top");
    ul_post_bg_bottom     = glGetUniformLocation(prog_post->id(), "u_bg_bottom");

    std::cout << "Shaders OK\n";
    return true;
}

void init_fbo(int w, int h) {
    if (g_fbo == 0) glGenFramebuffers(1, &g_fbo);
    auto mk_tex = [&](GLuint& t, GLint internal, GLenum format, GLenum type) {
        if (t == 0) glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };
    mk_tex(g_tex_color,  GL_RGBA8,             GL_RGBA,            GL_UNSIGNED_BYTE);
    mk_tex(g_tex_normal, GL_RGB16F,            GL_RGB,             GL_FLOAT);
    mk_tex(g_tex_depth,  GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_tex_color,  0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_tex_normal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, g_tex_depth,  0);
    GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, bufs);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "FBO incomplete\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initobjects() {
    model_mesh = Mesh::load_from_file("assets/maquette/scene.gltf");
    if (!model_mesh) {
        std::cerr << "ERREUR: assets/maquette/scene.gltf non chargé\n";
        return;
    }
    model_mesh->upload();

    float mn[3], mx[3];
    model_mesh->bounds(mn, mx);
    float cx = (mn[0]+mx[0]) * 0.5f, cy = (mn[1]+mx[1]) * 0.5f, cz = (mn[2]+mx[2]) * 0.5f;
    float ex = mx[0]-mn[0], ey = mx[1]-mn[1], ez = mx[2]-mn[2];
    float biggest = std::max(ex, std::max(ey, ez));
    if (biggest <= 0.0f) biggest = 1.0f;
    float s = 3.0f / biggest;
    g_fit = mygl::matrix4::scale(s, s, s) * mygl::matrix4::translate(-cx, -cy, -cz);

    std::cout << "Modèle: " << model_mesh->vertex_count() << " verts, "
              << "bbox [" << ex << " x " << ey << " x " << ez << "], échelle " << s << "\n";

    cam.center  = {0, 0, 0};
    cam.distance = 3.6f;
    cam.angle_v  = 0.30f;
}

void initPOV() {
    g_projection = mygl::frustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 150.0f);
}

void timer(int) {
    if (g_animating) {
        if (g_spin) g_time += 0.016f;
        glutPostRedisplay();
    }
    glutTimerFunc(16, timer, 0);
}

void display() {
    mygl::vec3 eye = cam.get_eye();
    const Style& s = g_styles[g_style];

    mygl::matrix4 model = mygl::matrix4::rotate_y(g_time * 0.3f) * g_fit;

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, g_win_w, g_win_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK();
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    prog_cel->use();

    glUniform3f(ul_light,    0.35f, 0.90f, 0.30f);
    glUniform3f(ul_lightcol, 1.2f, 1.2f, 1.2f);
    glUniform3f(ul_eye,      eye.x, eye.y, eye.z);
    glUniform1f(ul_bands,    g_num_bands);
    glUniform1f(ul_ambient,  s.ambient);
    glUniform1f(ul_spec_thr, s.spec_thr);
    glUniform1f(ul_spec_str, s.spec_str);
    glUniform1i(ul_use_hatch, g_hatch ? 1 : 0);

    mygl::matrix4 mv = cam.get_view() * model;
    glUniformMatrix4fv(ul_model_cel, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(ul_mv_cel,    1, GL_FALSE, mv.data());
    glUniformMatrix4fv(ul_proj_cel,  1, GL_FALSE, g_projection.data());
    if (model_mesh)
        model_mesh->draw_colored(ul_use_tex, ul_obj_color);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_win_w, g_win_h);
    glDisable(GL_DEPTH_TEST);
    prog_post->use();

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_tex_color);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_tex_normal);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_tex_depth);

    glUniform2f(ul_post_texel, 1.0f / g_win_w, 1.0f / g_win_h);
    glUniform1f(ul_post_depth_thr,  0.40f);
    glUniform1f(ul_post_normal_thr, 0.45f);
    glUniform1f(ul_post_dist_near, 3.0f);
    glUniform1f(ul_post_dist_far, 12.0f);
    glUniform3fv(ul_post_color, 1, s.outline);
    glUniform3fv(ul_post_color_far, 1, s.outline_far);
    glUniform1f(ul_post_thickness, s.thickness);
    glUniform1f(ul_post_thickness_far, s.thickness_far);
    glUniform3fv(ul_post_bg_top, 1, s.bg_top);
    glUniform3fv(ul_post_bg_bottom, 1, s.bg_bottom);

    glBindVertexArray(g_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    g_win_w = w; g_win_h = h;
    glViewport(0, 0, w, h); CHECK();
    init_fbo(w, h);
}

void keyboard(unsigned char k, int, int) {
    switch (k) {
        case '+': case '=':
            g_num_bands = std::min(g_num_bands + 1.0f, 16.0f);
            std::cout << "num_bands = " << g_num_bands << "\n"; glutPostRedisplay(); break;
        case '-': case '_':
            g_num_bands = std::max(g_num_bands - 1.0f, 1.0f);
            std::cout << "num_bands = " << g_num_bands << "\n"; glutPostRedisplay(); break;
        case ' ':
            g_spin = !g_spin;
            std::cout << (g_spin ? "Rotation ON\n" : "Rotation OFF\n"); break;
        case 'h': case 'H':
            g_hatch = !g_hatch;
            std::cout << (g_hatch ? "Hachures ON\n" : "Hachures OFF\n");
            glutPostRedisplay(); break;
        case '1': case '2': case '3': {
            int i = k - '1';
            if (i < g_style_count) apply_style(i);
            break;
        }
        default:
            cam.on_key(k);
    }
}

int main(int argc, char* argv[]) {
    if (!initglut(argc, argv)) return 1;
    if (!initglew())           return 1;
    initGL();
    if (!initshaders())        return 1;
    initobjects();
    initPOV();

    glGenVertexArrays(1, &g_quad_vao);
    init_fbo(g_win_w, g_win_h);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc([](int k, int, int){ cam.on_special(k); });
    glutTimerFunc(16, timer, 0);

    std::cout << "WASD/fleches=orbite  Q/E=zoom  IJKL=pan  +/-=bandes  H=hachures  1-3=styles  ESPACE=rotation  ESC=quitter\n";
    std::cout << "Styles :";
    for (int i = 0; i < g_style_count; ++i)
        std::cout << "  " << (i + 1) << "=" << g_styles[i].name;
    std::cout << "\n";
    apply_style(0);
    glutMainLoop();
}
