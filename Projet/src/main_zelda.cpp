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

#define CHECK()                                                            \
    do                                                                     \
    {                                                                      \
        GLenum e = glGetError();                                           \
        if (e != GL_NO_ERROR)                                              \
            std::cerr << "GL error " << e << " line " << __LINE__ << "\n"; \
    } while (0)

// ─── état global ──────────────────────────────────────────────────────────
program *prog_cel = nullptr;
program *prog_post = nullptr;
program *prog_shadow = nullptr;

Mesh *link_body = nullptr;
Mesh *ground = nullptr;
Mesh *boat = nullptr;
Texture *tex_water = nullptr;

Camera cam;

// ─── FBO ──────────────────────────────────────────────────────────────────
GLuint g_fbo = 0;
GLuint g_tex_color = 0;
GLuint g_tex_normal = 0;
GLuint g_tex_depth = 0;
GLuint g_quad_vao = 0;
int g_win_w = 1024;
int g_win_h = 1024;

const float GROUND_Y = -1.3f;

float g_link_scale = 0.2111f;
float g_boat_scale = 1.9580f;
float g_boat_y = -1.20f;
float g_link_y = 0.5f;
float g_link_x = 7.15f;
float g_link_z = 0.0f;

mygl::matrix4 g_projection;

float g_time = 0.0f;
bool g_animating = true;
float g_timescale = 1.0f;   // vitesse du cycle jour/nuit (touche T)
float g_num_bands = 4.0f;
bool g_hatch = true;

// ─── Presets de style ─────────────────────────────────────────────────────
struct Style
{
    const char *name;
    float num_bands;
    bool hatch;
    float ambient, spec_thr, spec_str;
    float outline[3], outline_far[3];
    float thickness, thickness_far;
    float bg_top[3], bg_bottom[3];
};

const Style g_styles[] = {
    {"BD cartoon", 4.f, true, .15f, .55f, .70f, {0, 0, 0}, {.22f, .28f, .40f}, 5.f, .35f, {.52f, .70f, .92f}, {.93f, .91f, .82f}},
    {"Croquis encre", 3.f, true, .28f, .70f, .00f, {.10f, .07f, .05f}, {.40f, .34f, .28f}, 4.f, .45f, {.90f, .86f, .76f}, {.80f, .74f, .62f}},
    {"Pop comics", 5.f, false, .22f, .50f, .95f, {0, 0, 0}, {0, 0, 0}, 7.f, .60f, {.25f, .62f, .96f}, {.99f, .85f, .38f}},
    {"Phong (compar.)", 64.f, false, .12f, .40f, .85f, {0, 0, 0}, {0, 0, 0}, 0.f, 1.0f, {.80f, .82f, .86f}, {.60f, .62f, .66f}},
};
const int g_style_count = sizeof(g_styles) / sizeof(g_styles[0]);
int g_style = 0;

static void apply_style(int i)
{
    g_style = i;
    g_num_bands = g_styles[i].num_bands;
    g_hatch = g_styles[i].hatch;
    std::cout << "Style " << (i + 1) << " : " << g_styles[i].name << "\n";
    glutPostRedisplay();
}

// ─── Uniforms cel ─────────────────────────────────────────────────────────
GLint ul_model_cel;
GLint ul_mv_cel, ul_proj_cel, ul_light, ul_lightcol, ul_eye;
GLint ul_bands, ul_ambient, ul_spec_thr, ul_spec_str;
GLint ul_use_tex, ul_obj_color, ul_use_hatch;
GLint ul_uv_scale; // ← global correct

// Uniforms post-process
GLint ul_post_texel, ul_post_color, ul_post_depth_thr, ul_post_normal_thr, ul_post_thickness;
GLint ul_post_color_far, ul_post_thickness_far, ul_post_dist_near, ul_post_dist_far;
GLint ul_post_bg_top, ul_post_bg_bottom;

// Uniforms ombre
GLint ul_shadow_mvp, ul_shadow_color, ul_shadow_gnormal;

// ─── helper ───────────────────────────────────────────────────────────────
static std::string read_file(const std::string &path)
{
    std::ifstream f(path);
    if (!f)
    {
        std::cerr << "Cannot open: " << path << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ─── init ─────────────────────────────────────────────────────────────────
bool initglut(int &argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitContextVersion(4, 5);
    glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(10, 10);
    glutCreateWindow("Rendu Cartoon — POGL");
    return true;
}

bool initglew()
{
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "glewInit failed\n";
        return false;
    }
    return true;
}

void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.85f, 0.85f, 0.9f, 1.0f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

bool initshaders()
{
    prog_cel = program::make_program(
        read_file("shaders/cel_vertex.glsl"),
        read_file("shaders/cel_fragment.glsl"));
    if (!prog_cel || !prog_cel->is_ready())
    {
        std::cerr << "cel shader failed\n";
        return false;
    }
    prog_post = program::make_program(
        read_file("shaders/post_vertex.glsl"),
        read_file("shaders/post_fragment.glsl"));
    if (!prog_post || !prog_post->is_ready())
    {
        std::cerr << "post shader failed\n";
        return false;
    }
    prog_shadow = program::make_program(
        read_file("shaders/shadow_vertex.glsl"),
        read_file("shaders/shadow_fragment.glsl"));
    if (!prog_shadow || !prog_shadow->is_ready())
    {
        std::cerr << "shadow shader failed\n";
        return false;
    }

    // ── Uniforms cel ──
    prog_cel->use();
    ul_model_cel = glGetUniformLocation(prog_cel->id(), "model");
    ul_mv_cel = glGetUniformLocation(prog_cel->id(), "model_view");
    ul_proj_cel = glGetUniformLocation(prog_cel->id(), "projection");
    ul_light = glGetUniformLocation(prog_cel->id(), "light_dir");
    ul_lightcol = glGetUniformLocation(prog_cel->id(), "light_color");
    ul_eye = glGetUniformLocation(prog_cel->id(), "eye_position");
    ul_bands = glGetUniformLocation(prog_cel->id(), "num_bands");
    ul_ambient = glGetUniformLocation(prog_cel->id(), "ambient");
    ul_spec_thr = glGetUniformLocation(prog_cel->id(), "spec_threshold");
    ul_spec_str = glGetUniformLocation(prog_cel->id(), "spec_strength");
    ul_use_tex = glGetUniformLocation(prog_cel->id(), "use_texture");
    ul_obj_color = glGetUniformLocation(prog_cel->id(), "object_color");
    ul_use_hatch = glGetUniformLocation(prog_cel->id(), "use_hatch");
    ul_uv_scale = glGetUniformLocation(prog_cel->id(), "uv_scale"); // ← correct
    glUniform1i(glGetUniformLocation(prog_cel->id(), "tex_diffuse"), 0);
    glUniform1f(ul_uv_scale, 1.0f); // ← après prog_cel->use()

    // ── Uniforms post-process ──
    prog_post->use();
    glUniform1i(glGetUniformLocation(prog_post->id(), "u_color"), 0);
    glUniform1i(glGetUniformLocation(prog_post->id(), "u_normal"), 1);
    glUniform1i(glGetUniformLocation(prog_post->id(), "u_depth"), 2);
    ul_post_texel = glGetUniformLocation(prog_post->id(), "u_texel");
    ul_post_color = glGetUniformLocation(prog_post->id(), "u_outline_color");
    ul_post_depth_thr = glGetUniformLocation(prog_post->id(), "u_depth_thr");
    ul_post_normal_thr = glGetUniformLocation(prog_post->id(), "u_normal_thr");
    ul_post_thickness = glGetUniformLocation(prog_post->id(), "u_thickness");
    ul_post_color_far = glGetUniformLocation(prog_post->id(), "u_outline_color_far");
    ul_post_thickness_far = glGetUniformLocation(prog_post->id(), "u_thickness_far");
    ul_post_dist_near = glGetUniformLocation(prog_post->id(), "u_dist_near");
    ul_post_dist_far = glGetUniformLocation(prog_post->id(), "u_dist_far");
    ul_post_bg_top = glGetUniformLocation(prog_post->id(), "u_bg_top");
    ul_post_bg_bottom = glGetUniformLocation(prog_post->id(), "u_bg_bottom");

    // ── Uniforms ombre ──
    prog_shadow->use();
    ul_shadow_mvp = glGetUniformLocation(prog_shadow->id(), "mvp");
    ul_shadow_color = glGetUniformLocation(prog_shadow->id(), "shadow_color");
    ul_shadow_gnormal = glGetUniformLocation(prog_shadow->id(), "ground_view_normal");

    std::cout << "Shaders OK\n";
    return true;
}

// ─── FBO ──────────────────────────────────────────────────────────────────
void init_fbo(int w, int h)
{
    if (g_fbo == 0)
        glGenFramebuffers(1, &g_fbo);
    auto mk_tex = [&](GLuint &t, GLint internal, GLenum format, GLenum type)
    {
        if (t == 0)
            glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };
    mk_tex(g_tex_color, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    mk_tex(g_tex_normal, GL_RGB16F, GL_RGB, GL_FLOAT);
    mk_tex(g_tex_depth, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_tex_color, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_tex_normal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_tex_depth, 0);
    GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, bufs);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "FBO incomplete\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initobjects()
{
    ground = Mesh::make_plane(50.0f, GROUND_Y); // sol agrandi
    ground->upload();

    tex_water = Texture::load("assets/eau_windwaker.png");
    if (!tex_water || !tex_water->valid())
        fprintf(stderr, "ERREUR: eau_windwaker.png non chargé\n");
    else
        printf("OK: water texture chargée\n");

    link_body = Mesh::load_from_file("assets/Toon Link/toonlink.obj");
    if (!link_body)
        fprintf(stderr, "ERREUR: toonlink.obj non chargé\n");
    else
        printf("OK: toonlink %d verts\n", link_body->vertex_count());
    if (link_body)
        link_body->upload();

    boat = Mesh::load_from_file("assets/red_lion/scene.gltf");
    if (!boat)
        fprintf(stderr, "ERREUR: bateau non chargé\n");
    else
        printf("OK: boat %d verts\n", boat->vertex_count());
    if (boat)
        boat->upload();
}

void initPOV()
{
    g_projection = mygl::frustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 100.0f);
}

void timer(int)
{
    if (g_animating)
    {
        g_time += 0.016f * g_timescale;
        glutPostRedisplay();
    }
    glutTimerFunc(16, timer, 0);
}

// ─── draw helpers ─────────────────────────────────────────────────────────
static void draw_cel(Mesh *mesh, const mygl::matrix4 &model)
{
    mygl::matrix4 mv = cam.get_view() * model;
    glUniformMatrix4fv(ul_model_cel, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(ul_mv_cel, 1, GL_FALSE, mv.data());
    glUniformMatrix4fv(ul_proj_cel, 1, GL_FALSE, g_projection.data());
    mesh->draw();
}

static void draw_link(const mygl::matrix4 &model)
{
    if (!link_body)
        return;
    mygl::matrix4 mv = cam.get_view() * model;
    glUniformMatrix4fv(ul_model_cel, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(ul_mv_cel, 1, GL_FALSE, mv.data());
    glUniformMatrix4fv(ul_proj_cel, 1, GL_FALSE, g_projection.data());
    link_body->draw_textured(ul_use_tex);
}

static void draw_boat(const mygl::matrix4 &model)
{
    if (!boat)
        return;
    mygl::matrix4 mv = cam.get_view() * model;
    glUniformMatrix4fv(ul_model_cel, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(ul_mv_cel, 1, GL_FALSE, mv.data());
    glUniformMatrix4fv(ul_proj_cel, 1, GL_FALSE, g_projection.data());
    boat->draw_textured(ul_use_tex);
}

// ─── display ──────────────────────────────────────────────────────────────
void display()
{
    mygl::vec3 eye = cam.get_eye();
    const Style &s = g_styles[g_style];

    // ── PASSE 1 : scène → FBO ──
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, g_win_w, g_win_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK();
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    prog_cel->use();

    // ── Cycle jour/nuit (porté du binaire désert) : le soleil tourne ──
    auto lerp = [](float a, float b, float t){ return a + (b - a) * t; };
    const float DAYLEN = 120.0f;
    float theta = g_time * (6.2831853f / DAYLEN) + 1.3f;   // démarre de jour
    mygl::vec3 light = mygl::vec3(std::cos(theta), std::sin(theta), 0.35f).normalize();
    float dayf = std::sin(theta) * 1.6f + 0.30f; dayf = dayf < 0 ? 0 : (dayf > 1 ? 1 : dayf);
    float lcr = lerp(0.30f,1.00f,dayf), lcg = lerp(0.36f,0.97f,dayf), lcb = lerp(0.55f,0.90f,dayf);
    float ambient_dn = lerp(0.05f, 0.17f, dayf);
    float skT_r=lerp(0.05f,0.40f,dayf), skT_g=lerp(0.07f,0.62f,dayf), skT_b=lerp(0.16f,0.92f,dayf);
    float skB_r=lerp(0.10f,0.86f,dayf), skB_g=lerp(0.12f,0.82f,dayf), skB_b=lerp(0.22f,0.70f,dayf);
    glUniform3f(ul_light, light.x, light.y, light.z);
    glUniform3f(ul_lightcol, lcr, lcg, lcb);
    glUniform3f(ul_eye, eye.x, eye.y, eye.z);
    glUniform1f(ul_bands, g_num_bands);
    glUniform1f(ul_ambient, ambient_dn);
    glUniform1f(ul_spec_thr, s.spec_thr);
    glUniform1f(ul_spec_str, s.spec_str);
    glUniform1i(ul_use_hatch, g_hatch ? 1 : 0);

    // ── Sol avec texture eau ──
    if (tex_water && tex_water->valid())
    {
        glUniform1i(ul_use_tex, 1);
        glUniform1f(ul_uv_scale, 20.0f);
        tex_water->bind(GL_TEXTURE0);
    }
    else
    {
        glUniform1i(ul_use_tex, 0);
        glUniform1f(ul_uv_scale, 1.0f);
        glUniform4f(ul_obj_color, 0.05f, 0.45f, 0.75f, 1.0f); // bleu fallback
    }
    draw_cel(ground, mygl::matrix4::identity());

    // Nettoie la texture du sol avant Link/bateau
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform1i(ul_use_tex, 0);
    glUniform1f(ul_uv_scale, 1.0f);

    // ── Ombres ──
    mygl::matrix4 view = cam.get_view();
    mygl::matrix4 shadowMat = mygl::matrix4::shadow(0, 1, 0, -GROUND_Y, light, 0.0f);
    mygl::matrix4 vp = g_projection * view;
    const GLfloat *vd = view.data();

    prog_shadow->use();
    glUniform3f(ul_shadow_color, 0.22f, 0.24f, 0.28f);
    glUniform3f(ul_shadow_gnormal, vd[4], vd[5], vd[6]);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // ── Link et bateau (avec leurs textures) ──
    prog_cel->use();
    mygl::matrix4 model_link = mygl::matrix4::translate(g_link_x, g_boat_y + g_link_y, g_link_z) * mygl::matrix4::scale(g_link_scale, g_link_scale, g_link_scale);
    mygl::matrix4 model_boat = mygl::matrix4::translate(0.0f, g_boat_y, 0.0f) * mygl::matrix4::scale(g_boat_scale, g_boat_scale, g_boat_scale);
    draw_link(model_link);
    draw_boat(model_boat);

    // ── PASSE 2 : contour post-process → écran ──
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_win_w, g_win_h);
    glDisable(GL_DEPTH_TEST);
    prog_post->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_tex_color);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_tex_normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_tex_depth);

    glUniform2f(ul_post_texel, 1.0f / g_win_w, 1.0f / g_win_h);
    glUniform1f(ul_post_depth_thr, 0.30f);
    glUniform1f(ul_post_normal_thr, 0.40f);
    glUniform1f(ul_post_dist_near, 3.0f);
    glUniform1f(ul_post_dist_far, 12.0f);
    glUniform3fv(ul_post_color, 1, s.outline);
    glUniform3fv(ul_post_color_far, 1, s.outline_far);
    glUniform1f(ul_post_thickness, s.thickness);
    glUniform1f(ul_post_thickness_far, s.thickness_far);
    glUniform3f(ul_post_bg_top, skT_r, skT_g, skT_b);       // ciel jour/nuit
    glUniform3f(ul_post_bg_bottom, skB_r, skB_g, skB_b);

    glBindVertexArray(g_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    g_win_w = w;
    g_win_h = h;
    glViewport(0, 0, w, h);
    CHECK();
    init_fbo(w, h);
}

// ─── clavier ──────────────────────────────────────────────────────────────
void keyboard(unsigned char k, int, int)
{
    switch (k)
    {
    case '+':
    case '=':
        g_num_bands = std::min(g_num_bands + 1.0f, 16.0f);
        std::cout << "num_bands = " << g_num_bands << "\n";
        glutPostRedisplay();
        break;
    case '-':
    case '_':
        g_num_bands = std::max(g_num_bands - 1.0f, 1.0f);
        std::cout << "num_bands = " << g_num_bands << "\n";
        glutPostRedisplay();
        break;
    case 'p':
    case 'P':
        g_animating = !g_animating;
        std::cout << (g_animating ? "Animation ON\n" : "Animation OFF\n");
        break;
    case 'h':
    case 'H':
        g_hatch = !g_hatch;
        std::cout << (g_hatch ? "Hachures ON\n" : "Hachures OFF\n");
        glutPostRedisplay();
        break;
    case 't':
    case 'T':
        g_timescale = (g_timescale < 2.0f) ? 8.0f : (g_timescale < 16.0f ? 30.0f : 1.0f);
        std::cout << "Cycle jour/nuit x" << g_timescale << "\n";
        break;
    case '1':
    case '2':
    case '3':
    case '4':
    {
        int i = k - '1';
        if (i < g_style_count)
            apply_style(i);
        break;
    }
    case 'u':
        g_link_scale *= 1.1f;
        printf("link scale=%.4f\n", g_link_scale);
        glutPostRedisplay();
        break;
    case 'j':
        g_link_scale *= 0.9f;
        printf("link scale=%.4f\n", g_link_scale);
        glutPostRedisplay();
        break;
    case 'o':
        g_boat_scale *= 1.1f;
        printf("boat scale=%.4f\n", g_boat_scale);
        glutPostRedisplay();
        break;
    case 'l':
        g_boat_scale *= 0.9f;
        printf("boat scale=%.4f\n", g_boat_scale);
        glutPostRedisplay();
        break;
    case 'i':
        g_link_y += 0.05f;
        printf("link y=%.2f\n", g_link_y);
        glutPostRedisplay();
        break;
    case 'k':
        g_link_y -= 0.05f;
        printf("link y=%.2f\n", g_link_y);
        glutPostRedisplay();
        break;
    case 'n':
        g_link_z += 0.05f;
        printf("link z=%.2f\n", g_link_z);
        glutPostRedisplay();
        break;
    case 'm':
        g_link_z -= 0.05f;
        printf("link z=%.2f\n", g_link_z);
        glutPostRedisplay();
        break;
    case 'b':
        g_link_x += 0.05f;
        printf("link x=%.2f\n", g_link_x);
        glutPostRedisplay();
        break;
    case 'v':
        g_link_x -= 0.05f;
        printf("link x=%.2f\n", g_link_x);
        glutPostRedisplay();
        break;
    default:
        cam.on_key(k);
    }
}

// ─── main ─────────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    if (!initglut(argc, argv))
        return 1;
    if (!initglew())
        return 1;
    initGL();
    if (!initshaders())
        return 1;
    initobjects();
    initPOV();

    glGenVertexArrays(1, &g_quad_vao);
    init_fbo(g_win_w, g_win_h);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc([](int k, int, int)
                    { cam.on_special(k); });
    glutTimerFunc(16, timer, 0);

    std::cout << "Fleches=orbite  A/E=zoom  ZQSD=deplacer  ESPACE/C=monter-descendre\n"
              << "+/-=bandes  H=hachures  1-4=styles  P=pause  ESC=quitter\n"
              << "u/j=scale link  o/l=scale bateau  i/k=link Y  n/m=link Z  b/v=link X\n";
    std::cout << "Styles :";
    for (int i = 0; i < g_style_count; ++i)
        std::cout << "  " << (i + 1) << "=" << g_styles[i].name;
    std::cout << "\n";
    glutMainLoop();
}