#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "matrix4.hh"
#include "program.hh"
#include "mesh.hh"
#include "camera.hh"

#define CHECK()                                                             \
  do { GLenum e=glGetError();                                              \
    if(e!=GL_NO_ERROR) std::cerr<<"GL error "<<e<<" line "<<__LINE__<<"\n";\
  } while(0)

// ─── état global ──────────────────────────────────────────────────────────
program* prog_cel    = nullptr;
program* prog_post   = nullptr;  // passe de contour post-process
program* prog_shadow = nullptr;  // ombre projetée planaire

Mesh*    sphere    = nullptr;
Mesh*    cylinder  = nullptr;  // avec caps
Mesh*    torus     = nullptr;
Mesh*    ground    = nullptr;  // sol

Camera   cam;

// ─── FBO (rendu hors-écran : scène + G-buffer normales/profondeur) ──────────
GLuint g_fbo        = 0;
GLuint g_tex_color  = 0;  // scène cel-shadée
GLuint g_tex_normal = 0;  // normales espace caméra
GLuint g_tex_depth  = 0;  // profondeur
GLuint g_quad_vao   = 0;  // VAO vide pour le triangle plein écran
int    g_win_w      = 1024;
int    g_win_h      = 1024;

// Couleurs unies par objet (textures retirées — on travaille en aplats)
const float COL_SPHERE[3]   = { 0.85f, 0.40f, 0.13f };  // orange
const float COL_CYLINDER[3] = { 0.27f, 0.55f, 0.30f };  // vert
const float COL_TORUS[3]    = { 0.20f, 0.35f, 0.70f };  // bleu
const float COL_GROUND[3]   = { 0.72f, 0.74f, 0.66f };  // sol beige-vert

const float GROUND_Y = -1.3f;   // hauteur du sol (plan de l'ombre)

mygl::matrix4 g_projection;

// Animation
float g_time      = 0.0f;
bool  g_animating = true;

// Paramètre cartoon interactif
float g_num_bands = 4.0f;  // toggle +/-
bool  g_hatch     = true;  // hachures de croquis (toggle 'h')

// Uniforms cel
GLint ul_model_cel;  // matrice model seule (espace monde pour éclairage)
GLint ul_mv_cel, ul_proj_cel, ul_light, ul_lightcol, ul_eye;
GLint ul_bands, ul_ambient, ul_spec_thr, ul_spec_str;
GLint ul_use_tex, ul_obj_color, ul_use_hatch;

// Uniforms post-process (contour + fond)
GLint ul_post_texel, ul_post_color, ul_post_depth_thr, ul_post_normal_thr, ul_post_thickness;
GLint ul_post_color_far, ul_post_thickness_far, ul_post_dist_near, ul_post_dist_far;
GLint ul_post_bg_top, ul_post_bg_bottom;

// Uniforms ombre projetée
GLint ul_shadow_mvp, ul_shadow_color, ul_shadow_gnormal;

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
    prog_post = program::make_program(
        read_file("shaders/post_vertex.glsl"),
        read_file("shaders/post_fragment.glsl"));
    if (!prog_post || !prog_post->is_ready()) {
        std::cerr << "post shader failed\n"; return false;
    }
    prog_shadow = program::make_program(
        read_file("shaders/shadow_vertex.glsl"),
        read_file("shaders/shadow_fragment.glsl"));
    if (!prog_shadow || !prog_shadow->is_ready()) {
        std::cerr << "shadow shader failed\n"; return false;
    }

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

    // Post-process : lier les samplers aux unités de texture + récupérer les seuils
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
    ul_post_bg_top     = glGetUniformLocation(prog_post->id(), "u_bg_top");
    ul_post_bg_bottom  = glGetUniformLocation(prog_post->id(), "u_bg_bottom");

    prog_shadow->use();
    ul_shadow_mvp     = glGetUniformLocation(prog_shadow->id(), "mvp");
    ul_shadow_color   = glGetUniformLocation(prog_shadow->id(), "shadow_color");
    ul_shadow_gnormal = glGetUniformLocation(prog_shadow->id(), "ground_view_normal");

    std::cout << "Shaders OK\n";
    return true;
}

// ─── FBO : (re)création des textures cibles à la taille de la fenêtre ────────
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
    sphere   = Mesh::make_sphere(48, 64, 1.0f);
    cylinder = Mesh::make_cylinder(64, 0.5f, 1.5f, true);  // caps pour le rendu
    torus    = Mesh::make_torus(48, 64, 0.65f, 0.28f);
    ground   = Mesh::make_plane(12.0f, GROUND_Y);          // grand sol (ombres restent dessus)
    sphere->upload();
    cylinder->upload();
    torus->upload();
    ground->upload();
    std::cout << "Sphere: "   << sphere->vertex_count()   << " verts\n";
    std::cout << "Cylinder: " << cylinder->vertex_count() << " verts\n";
    std::cout << "Torus: "    << torus->vertex_count()    << " verts\n";
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

// Dessine l'ombre aplatie d'un mesh (mvp = projection·view·shadow·model)
static void draw_shadow(Mesh* mesh, const mygl::matrix4& mvp) {
    glUniformMatrix4fv(ul_shadow_mvp, 1, GL_FALSE, mvp.data());
    mesh->draw();
}

// ─── boucle de rendu ──────────────────────────────────────────────────────
void display() {
    mygl::vec3 eye = cam.get_eye();

    // Matrices model avec rotation temporelle — vitesses différentes par objet
    mygl::matrix4 model_sphere   = mygl::matrix4::translate(-2.5f, 0, 0)
                                 * mygl::matrix4::rotate_y(g_time * 0.7f);
    mygl::matrix4 model_cylinder = mygl::matrix4::translate( 0.0f, 0, 0)
                                 * mygl::matrix4::rotate_y(g_time * 1.0f);
    mygl::matrix4 model_torus    = mygl::matrix4::translate( 2.5f, 0, 0)
                                 * mygl::matrix4::rotate_y(g_time * 0.5f)
                                 * mygl::matrix4::rotate_x(g_time * 0.3f);

    // ── PASSE 1 : SCÈNE CEL-SHADÉE → FBO (couleur + normales + profondeur) ──
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, g_win_w, g_win_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK();
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    prog_cel->use();

    // Lumière directionnelle fixe (soleil) : direction VERS la lumière.
    // Directionnel → dot(n,l) uniforme sur les faces plates (sol, capuchons).
    mygl::vec3 light = mygl::vec3(0.4f, 0.85f, 0.5f).normalize();
    glUniform3f(ul_light,    light.x, light.y, light.z);
    glUniform3f(ul_lightcol, 1.0f, 1.0f, 1.0f);
    glUniform3f(ul_eye,      eye.x, eye.y, eye.z);

    glUniform1f(ul_bands,    g_num_bands);
    glUniform1f(ul_ambient,  0.15f);
    glUniform1f(ul_spec_thr, 0.55f);
    glUniform1f(ul_spec_str, 0.7f);

    // Aplats : aucune texture, couleur unie par objet
    glUniform1i(ul_use_tex, 0);
    glUniform1i(ul_use_hatch, g_hatch ? 1 : 0);

    // Sol (cel-shadé)
    glUniform4f(ul_obj_color, COL_GROUND[0], COL_GROUND[1], COL_GROUND[2], 1.0f);
    draw_cel(ground, mygl::matrix4::identity());

    // ── Ombres projetées (entre le sol et les objets) ──────────────────────
    // Matrice d'ombre : aplatit sur le plan du sol y = GROUND_Y depuis la lampe.
    // Polygon offset : tire l'ombre vers la caméra pour qu'elle gagne le z-test
    // contre le sol coplanaire (sinon z-fighting).
    mygl::matrix4 view      = cam.get_view();
    // Ombre directionnelle (lw=0) : projection parallèle suivant la direction lumière
    mygl::matrix4 shadowMat = mygl::matrix4::shadow(0, 1, 0, -GROUND_Y, light, 0.0f);
    mygl::matrix4 vp        = g_projection * view;
    const GLfloat* vd = view.data();  // colonne 1 du 3×3 = normale +Y en espace caméra

    prog_shadow->use();
    glUniform3f(ul_shadow_color,   0.22f, 0.24f, 0.28f);
    glUniform3f(ul_shadow_gnormal, vd[4], vd[5], vd[6]);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    draw_shadow(sphere,   vp * shadowMat * model_sphere);
    draw_shadow(cylinder, vp * shadowMat * model_cylinder);
    draw_shadow(torus,    vp * shadowMat * model_torus);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // ── Objets (cel-shadés) ────────────────────────────────────────────────
    prog_cel->use();  // uniforms cel (lumière, bandes…) toujours en place
    glUniform4f(ul_obj_color, COL_SPHERE[0], COL_SPHERE[1], COL_SPHERE[2], 1.0f);
    draw_cel(sphere, model_sphere);

    glUniform4f(ul_obj_color, COL_CYLINDER[0], COL_CYLINDER[1], COL_CYLINDER[2], 1.0f);
    draw_cel(cylinder, model_cylinder);

    glUniform4f(ul_obj_color, COL_TORUS[0], COL_TORUS[1], COL_TORUS[2], 1.0f);
    draw_cel(torus, model_torus);

    // ── PASSE 2 : CONTOUR POST-PROCESS → ÉCRAN ─────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_win_w, g_win_h);
    glDisable(GL_DEPTH_TEST);
    prog_post->use();

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_tex_color);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_tex_normal);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_tex_depth);

    glUniform2f(ul_post_texel, 1.0f / g_win_w, 1.0f / g_win_h);
    glUniform3f(ul_post_color, 0.0f, 0.0f, 0.0f);
    glUniform1f(ul_post_depth_thr,  0.30f);  // silhouettes (à monter si bruit, baisser si trous)
    glUniform1f(ul_post_normal_thr, 0.40f);  // plis internes (jonction cap/flanc)
    glUniform1f(ul_post_thickness,  5.0f);   // épaisseur du trait au premier plan (px)
    // Contour variable par profondeur : loin = plus fin + estompé vers le fond
    glUniform1f(ul_post_thickness_far, 0.35f);             // loin → 35% de l'épaisseur
    glUniform1f(ul_post_dist_near,     3.0f);              // distance « premier plan »
    glUniform1f(ul_post_dist_far,      12.0f);             // distance « arrière-plan »
    glUniform3f(ul_post_color_far, 0.22f, 0.28f, 0.40f);   // trait lointain bleu-gris estompé
    glUniform3f(ul_post_bg_top,    0.52f, 0.70f, 0.92f);  // bleu ciel cartoon
    glUniform3f(ul_post_bg_bottom, 0.93f, 0.91f, 0.82f);  // crème pâle (bas)

    glBindVertexArray(g_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    g_win_w = w;
    g_win_h = h;
    glViewport(0, 0, w, h); CHECK();
    init_fbo(w, h);          // recréer les cibles à la nouvelle taille
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
        // Pause animation (Espace est désormais « monter » pour la caméra)
        case 'p': case 'P':
            g_animating = !g_animating;
            std::cout << (g_animating ? "Animation ON\n" : "Animation OFF\n"); break;
        // Toggle hachures de croquis
        case 'h': case 'H':
            g_hatch = !g_hatch;
            std::cout << (g_hatch ? "Hachures ON\n" : "Hachures OFF\n");
            glutPostRedisplay(); break;
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
    initPOV();

    glGenVertexArrays(1, &g_quad_vao);  // VAO vide pour le triangle plein écran
    init_fbo(g_win_w, g_win_h);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc([](int k, int, int){ cam.on_special(k); });
    glutTimerFunc(16, timer, 0);

    std::cout << "Fleches=orbite  A/E=zoom  ZQSD=deplacer  ESPACE/C=monter-descendre  +/-=bandes  H=hachures  P=pause  ESC=quitter\n";
    glutMainLoop();
}
