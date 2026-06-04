// ─── Rendu Cartoon Temps Réel (POGL) ───────────────────────────────────────
// Point d'entrée : init GLUT/GLEW, boucle de rendu en 2 passes (cel shading +
// ombres → FBO, puis contour/fond en post-process), presets de style et clavier.
// Pipeline et techniques détaillés dans README.md et les PHASE_*.md.
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

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

// Scène « casse de Pandora » (Borderlands) — voir initobjects()
Mesh*    ground  = nullptr;  // sol désertique
Mesh*    barrel  = nullptr;  // baril explosif (cylindre)
Mesh*    tank    = nullptr;  // grosse cuve (cylindre couché)
Mesh*    crate   = nullptr;  // caisse (cube)
Mesh*    tire    = nullptr;  // pneu (tore)
Mesh*    cone    = nullptr;  // cône de chantier
Mesh*    rock    = nullptr;  // rocher (sphère)

Camera   cam;

// ─── FBO (rendu hors-écran : scène + G-buffer normales/profondeur) ──────────
GLuint g_fbo        = 0;
GLuint g_tex_color  = 0;  // scène cel-shadée
GLuint g_tex_normal = 0;  // normales espace caméra
GLuint g_tex_depth  = 0;  // profondeur
GLuint g_quad_vao   = 0;  // VAO vide pour le triangle plein écran
int    g_win_w      = 1024;
int    g_win_h      = 1024;

// Aplats de couleur (palette Borderlands : désert + props industriels)
const float COL_GROUND[3] = { 0.74f, 0.62f, 0.42f };  // sable / terre de Pandore

const float GROUND_Y = -1.3f;   // hauteur du sol (plan de l'ombre)

mygl::matrix4 g_projection;

// Animation
float g_time      = 0.0f;
bool  g_animating = true;

// Paramètre cartoon interactif
float g_num_bands = 4.0f;  // toggle +/-
bool  g_hatch     = true;  // hachures de croquis (toggle 'h')

// ─── Presets de style (Phase 5 : démo interactive, touches 1..4) ────────────
// Regroupe tous les réglages « look » : un appui = un changement de style complet.
// Le dernier (« Phong ») désactive contour + hachures et monte les bandes → sert
// de comparaison rendu réaliste ↔ cartoon pour la soutenance.
struct Style {
    const char* name;
    float num_bands;
    bool  hatch;
    float ambient, spec_thr, spec_str;
    float outline[3], outline_far[3];   // couleur du trait : premier plan / lointain
    float thickness, thickness_far;     // épaisseur (px) / facteur au loin
    float bg_top[3], bg_bottom[3];      // gradient de fond
};

const Style g_styles[] = {
    //  nom              bands hatch  amb   sthr  sstr   outline(près)      outline(loin)       th   far    bg_top              bg_bottom
    { "BD cartoon",      4.f,  true,  .15f, .55f, .70f, {0,0,0},           {.22f,.28f,.40f},   5.f, .35f, {.52f,.70f,.92f},   {.93f,.91f,.82f} },
    { "Croquis encre",   3.f,  true,  .28f, .70f, .00f, {.10f,.07f,.05f},  {.40f,.34f,.28f},   4.f, .45f, {.90f,.86f,.76f},   {.80f,.74f,.62f} },
    { "Pop comics",      5.f,  false, .22f, .50f, .95f, {0,0,0},           {0,0,0},            7.f, .60f, {.25f,.62f,.96f},   {.99f,.85f,.38f} },
    { "Phong (compar.)", 64.f, false, .12f, .40f, .85f, {0,0,0},           {0,0,0},            0.f, 1.0f, {.80f,.82f,.86f},   {.60f,.62f,.66f} },
};
const int g_style_count = sizeof(g_styles) / sizeof(g_styles[0]);
int g_style = 0;

// Applique un preset : recopie ses bandes/hachures dans les globals modifiables
// (pour que +/- et H restent un réglage fin par-dessus le style choisi).
static void apply_style(int i) {
    g_style     = i;
    g_num_bands = g_styles[i].num_bands;
    g_hatch     = g_styles[i].hatch;
    std::cout << "Style " << (i + 1) << " : " << g_styles[i].name << "\n";
    glutPostRedisplay();
}

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
    ground = Mesh::make_plane(12.0f, GROUND_Y);          // grand sol (ombres restent dessus)
    barrel = Mesh::make_cylinder(28, 0.42f, 1.05f, true);// baril
    tank   = Mesh::make_cylinder(32, 0.75f, 2.60f, true);// grosse cuve
    crate  = Mesh::make_box(0.55f, 0.55f, 0.55f);        // caisse cubique
    tire   = Mesh::make_torus(28, 40, 0.55f, 0.22f);     // pneu
    cone   = Mesh::make_cone(28, 0.50f, 1.05f);          // cône de chantier
    rock   = Mesh::make_sphere(32, 40, 1.0f);            // rocher (mis à l'échelle au rendu)
    for (Mesh* m : { ground, barrel, tank, crate, tire, cone, rock })
        m->upload();
    std::cout << "Scene Borderlands prete (7 meshes)\n";
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
    mygl::vec3   eye = cam.get_eye();
    const Style& s   = g_styles[g_style];   // preset de style courant

    // ── Composition de la scène (casse de Pandora) ─────────────────────────
    // Chaque instance = mesh + matrice model (posée au sol y=GROUND_Y) + couleur.
    // L'offset vertical = demi-hauteur de l'objet pour qu'il repose sur le sol.
    using M = mygl::matrix4;
    const float G  = GROUND_Y;
    const float Q  = 1.5708f;            // 90° pour coucher/dresser les objets
    struct Inst { Mesh* mesh; M model; float col[3]; };

    std::vector<Inst> scene = {
        // Grosse cuve couchée (cylindre sur le flanc) — métal teal
        { tank,   M::translate(-3.0f, G+0.75f, -1.1f) * M::rotate_y(0.3f) * M::rotate_z(Q),
                  {0.34f, 0.50f, 0.50f} },
        // Trois barils explosifs rouges
        { barrel, M::translate(-1.5f, G+0.525f,  0.8f),                      {0.80f,0.16f,0.12f} },
        { barrel, M::translate(-0.8f, G+0.525f, -0.2f) * M::rotate_y(0.5f),  {0.80f,0.16f,0.12f} },
        { barrel, M::translate(-1.9f, G+0.525f, -0.7f),                      {0.78f,0.20f,0.14f} },
        // Pile de caisses (cubes) — rouille + jaune danger
        { crate,  M::translate(1.6f,  G+0.55f,  0.6f)  * M::rotate_y(0.3f),  {0.80f,0.52f,0.20f} },
        { crate,  M::translate(1.55f, G+1.65f,  0.55f) * M::rotate_y(-0.2f), {0.85f,0.68f,0.18f} },
        { crate,  M::translate(2.5f,  G+0.55f, -0.1f)  * M::rotate_y(0.7f),  {0.78f,0.48f,0.16f} },
        // Pneu dressé
        { tire,   M::translate(2.4f,  G+0.77f,  1.2f)  * M::rotate_y(0.4f) * M::rotate_x(Q),
                  {0.16f, 0.16f, 0.18f} },
        // Cône de chantier
        { cone,   M::translate(0.6f,  G+0.525f, 1.7f),                       {0.90f,0.42f,0.12f} },
        // Rocher (sphère écrasée)
        { rock,   M::translate(3.4f,  G+0.50f,  0.9f)  * M::scale(0.7f,0.5f,0.7f), {0.46f,0.45f,0.42f} },
        // Mât + panneau qui TOURNE (seul élément animé) — repère « Catch-A-Ride »
        { barrel, M::translate(3.6f,  G+1.16f, -1.6f)  * M::scale(0.12f,2.2f,0.12f), {0.28f,0.28f,0.30f} },
        { crate,  M::translate(3.6f,  G+2.30f, -1.6f)  * M::rotate_y(g_time*0.7f) * M::scale(0.9f,0.5f,0.10f),
                  {0.88f, 0.70f, 0.15f} },
    };

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
    glUniform1f(ul_ambient,  s.ambient);
    glUniform1f(ul_spec_thr, s.spec_thr);
    glUniform1f(ul_spec_str, s.spec_str);

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
    glUniform3f(ul_shadow_color,   0.20f, 0.17f, 0.14f);  // ombre brune (sol désertique)
    glUniform3f(ul_shadow_gnormal, vd[4], vd[5], vd[6]);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    for (const Inst& it : scene)
        draw_shadow(it.mesh, vp * shadowMat * it.model);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // ── Objets (cel-shadés) ────────────────────────────────────────────────
    prog_cel->use();  // uniforms cel (lumière, bandes…) toujours en place
    for (const Inst& it : scene) {
        glUniform4f(ul_obj_color, it.col[0], it.col[1], it.col[2], 1.0f);
        draw_cel(it.mesh, it.model);
    }

    // ── PASSE 2 : CONTOUR POST-PROCESS → ÉCRAN ─────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_win_w, g_win_h);
    glDisable(GL_DEPTH_TEST);
    prog_post->use();

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_tex_color);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_tex_normal);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_tex_depth);

    glUniform2f(ul_post_texel, 1.0f / g_win_w, 1.0f / g_win_h);
    glUniform1f(ul_post_depth_thr,  0.30f);  // silhouettes (à monter si bruit, baisser si trous)
    glUniform1f(ul_post_normal_thr, 0.40f);  // plis internes (jonction cap/flanc)
    glUniform1f(ul_post_dist_near,  3.0f);   // distance « premier plan »
    glUniform1f(ul_post_dist_far,   12.0f);  // distance « arrière-plan »
    // Réglages « look » pilotés par le preset de style courant (thickness=0 → pas de contour)
    glUniform3fv(ul_post_color,     1, s.outline);
    glUniform3fv(ul_post_color_far, 1, s.outline_far);
    glUniform1f (ul_post_thickness,     s.thickness);
    glUniform1f (ul_post_thickness_far, s.thickness_far);
    glUniform3fv(ul_post_bg_top,    1, s.bg_top);
    glUniform3fv(ul_post_bg_bottom, 1, s.bg_bottom);

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
        // Presets de style (1..4) — dont « Phong » pour la comparaison
        case '1': case '2': case '3': case '4': {
            int i = k - '1';
            if (i < g_style_count) apply_style(i);
            break;
        }
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

    std::cout << "Fleches=orbite  A/E=zoom  ZQSD=deplacer  ESPACE/C=monter-descendre\n"
              << "+/-=bandes  H=hachures  1-4=styles  P=pause  ESC=quitter\n";
    std::cout << "Styles :";
    for (int i = 0; i < g_style_count; ++i)
        std::cout << "  " << (i + 1) << "=" << g_styles[i].name;
    std::cout << "\n";
    glutMainLoop();
}
