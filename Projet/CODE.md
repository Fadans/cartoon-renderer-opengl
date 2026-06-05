# Documentation du code

> Le code source est volontairement **sans commentaires** ; les explications sont
> regroupées ici. Vue d'ensemble du rendu → `CONTEXT.md` ; détail par phase →
> `PHASE_*.md`. Ce fichier décrit **le rôle de chaque fichier et de ses fonctions**.

## Organisation générale

Le projet produit **deux binaires** qui partagent la majorité du code :

| Binaire | `main` | Scène |
|---|---|---|
| `cartoon` | `src/main.cpp` | désert procédural (buttes, herbe, cailloux) + cycle jour/nuit |
| `zelda`   | `src/main_zelda.cpp` | modèles externes (Toon Link + bateau) chargés via Assimp |

Fichiers **partagés** : `matrix4`, `camera`, `program`, `mesh`, `texture` + les shaders.
`mesh_loader.cpp` (Assimp) n'est lié qu'à `zelda`. Pipeline de rendu en **2 passes** :
passe 1 = cel shading + ombres → FBO ; passe 2 = fond + contour en post-process.

---

## `matrix4.hh / .cpp` — maths (namespace `mygl`)

- `struct vec3` : vecteur 3D. `operator-`, `dot`, `cross`, `normalize`.
- `matrix4` : matrice 4×4 **column-major** (`m[col*4 + row]`).
  - `operator*`, `operator*=` : produit matriciel. `data()` : pointeur pour `glUniformMatrix4fv`.
  - `identity()`, `translate(tx,ty,tz)`, `rotate_x/y/z(angle)`, `scale(sx,sy,sz)` : transformations.
  - `shadow(a,b,c,d, light, lw)` : matrice d'**ombre projetée planaire** — aplatit la
    géométrie sur le plan `(a,b,c,d)` depuis `light` (`lw=0` = soleil directionnel).
- `lookat(eye, center, up)` : matrice de vue. `frustum(l,r,b,t,near,far)` : projection perspective.

## `camera.hh / .cpp` — caméra

Caméra orbitale (coordonnées sphériques `angle_h`, `angle_v`, `distance`) autour d'un
point `center` déplaçable.

- `get_eye()` : position de l'œil (calculée depuis les angles + `center`).
- `get_view()` : matrice de vue (`lookat` œil → `center`).
- `pan(dx,dy)` : déplace `center` dans le plan écran.
- `move(strafe, forward)` : déplacement **au sol** (avant projeté en XZ + strafe).
- `on_key(key)` : A/E = zoom, ZQSD = déplacer, Espace/C = monter/descendre, Échap = quitter.
- `on_special(key)` : flèches = orbite (gauche/droite/haut/bas).

## `program.hh / .cpp` — programmes shaders

- `make_program(vertex_src, fragment_src)` : compile les deux shaders, les linke en un
  programme, supprime les shaders intermédiaires. Renvoie un `program*`.
- `use()` : `glUseProgram`. `id()` : l'identifiant GL. `is_ready()` : compilation/link OK.

## `texture.hh / .cpp` — textures (via stb_image)

- `Texture::load(path)` : charge une image (PNG…) avec **stb_image** en texture GL.
- `Texture::checkerboard(...)` : génère un damier procédural (utilitaire).
- `bind(unit)` : lie la texture à une unité (`GL_TEXTURE0`…). `valid()` : chargement réussi.

## `mesh.hh / .cpp` — géométrie

`class Mesh` = un VAO + 3 VBO (positions, normales, UV) + une liste de `SubMesh`
(plages de sommets texturées, pour les modèles chargés).

**Générateurs procéduraux** (renvoient un `Mesh*`) :
- `make_sphere`, `make_cylinder` (avec caps), `make_torus`, `make_plane` : primitives de base.
- `make_box(hx,hy,hz)` : cube, normales par face.
- `make_cone(n,radius,height)` : cône (normale de flanc inclinée selon la pente).
- `make_rock(...)` : rocher facetté = sphère basse résolution déformée par bruit, normales plates.
- `make_butte(rings,sectors,radius,height,jitter,seed)` : **mesa** = rocher bruité dont le
  Y est étiré puis clampé → sommet et base plats, flancs rocheux.
- `make_mesa(...)` : variante prisme érodé (non utilisée par la scène actuelle).
- `make_grass(blades,height,spread,seed)` : touffe de brins (triangles fins double face).

**Méthodes d'instance** :
- `upload()` : envoie les VBO au GPU + charge la texture de chaque `SubMesh`.
- `draw()` : dessine tout le mesh (`glDrawArrays`).
- `draw_textured(ul_use_tex)` : dessine **sous-mesh par sous-mesh**, en liant la texture de chacun.
- `push_vertex(...)`, `push_submesh(...)` : API d'ajout utilisée par le loader de modèles.
- `vertex_count()` : nombre de sommets.
- `load_from_file(path)` : **implémentée dans `mesh_loader.cpp`** (voir ci-dessous).

## `mesh_loader.cpp` — chargement de modèles (Assimp) — voir section dédiée

- `Mesh::load_from_file(path)` : charge un modèle externe (OBJ/glTF/DAE) en un `Mesh`.
- `process_node(...)` (interne) : parcours récursif de la hiérarchie du modèle.

## `main.cpp` — application désert (`cartoon`)

- `read_file(path)` : lit un fichier shader en `std::string`.
- `initglut/initglew/initGL` : contexte OpenGL 4.5 core, états (depth test, culling…).
- `initshaders()` : compile les 3 programmes (cel, post, shadow) et récupère les *uniform locations*.
- `init_fbo(w,h)` : (re)crée le FBO et ses cibles (couleur, normales, profondeur) à la taille fenêtre.
- `initobjects()` : génère les meshes de la scène (buttes, herbe, cailloux, sol).
- `initPOV()` : matrice de projection (frustum). `timer()` : avance `g_time` (× `g_timescale`).
- `apply_style(i)` : applique un preset de style (bandes, hachures, couleurs…).
- `draw_cel(mesh, model)` / `draw_shadow(mesh, mvp, world)` : envoient les matrices et dessinent.
- `display()` : **cœur du rendu** — calcule le **cycle jour/nuit** (direction/couleur du
  soleil, ciel, ambiante selon `g_time`), construit la **liste d'instances** de la scène,
  fait la passe 1 (sol + objets cel + ombres → FBO) puis la passe 2 (fond + contour → écran).
- `reshape(w,h)` : viewport + recrée le FBO. `keyboard(...)` : presets, hachures, vitesse jour/nuit, pause.
- `main()` : enchaîne les init, règle la caméra, lance `glutMainLoop`.

## `main_zelda.cpp` — application Zelda (`zelda`)

Même squelette que `main.cpp`, mais :
- `initobjects()` charge **Toon Link** (`Mesh::load_from_file("assets/Toon Link/toonlink.obj")`)
  et le **bateau** (`assets/red_lion/scene.gltf`), + une texture d'eau pour le sol.
- `display()` dessine ces modèles avec `draw_textured` (textures par sous-mesh) et reprend
  le même cycle jour/nuit. Démarre en **style 3** et caméra tournée par défaut.
- Touches supplémentaires pour ajuster position/échelle des modèles (debug).

## Shaders (`shaders/*.glsl`)

- `cel_vertex` / `cel_fragment` : **cel shading** (diffus quantifié en N bandes + spéculaire
  seuillé) ; écrit la couleur **et** la normale (G-buffer) ; ajoute les **hachures** d'écran.
- `shadow_vertex` / `shadow_fragment` : **ombre projetée** ; calcule la position monde au sol
  et **clippe** l'ombre aux bornes du terrain (si `ground_half > 0`).
- `post_vertex` / `post_fragment` : passe plein écran ; **fond gradient** + **détection de
  contour** (Laplacien de profondeur ∪ saut de normale), trait à épaisseur/couleur variables.

---

## Assimp — chargement de modèles externes (détaillé)

**Assimp** (*Open Asset Import Library*) est une bibliothèque qui lit des dizaines de formats
3D (OBJ, glTF, DAE/Collada, FBX…) et les expose sous une **structure commune** en mémoire.
On l'utilise dans `zelda` pour charger Toon Link et le bateau, sans écrire un parseur par format.

### Ce qu'Assimp renvoie

`importer.ReadFile(path, flags)` renvoie un `aiScene*` :
- `scene->mRootNode` : la **racine** d'un arbre de nœuds (`aiNode`). Chaque nœud a une
  **transformation locale** (`mTransformation`) et des enfants → on accumule les transformations
  du parent vers l'enfant.
- `scene->mMeshes[]` : les **maillages** (`aiMesh`) : sommets (`mVertices`), normales
  (`mNormals`), coordonnées de texture (`mTextureCoords[0]`), faces (`mFaces`, indices).
- `scene->mMaterials[]` : les **matériaux** ; on y lit la **texture diffuse**
  (`GetTexture(aiTextureType_DIFFUSE, …)`).

### Les flags de post-traitement (à `ReadFile`)

- `aiProcess_Triangulate` : convertit tous les polygones en **triangles** (indispensable, on
  dessine en `GL_TRIANGLES`).
- `aiProcess_GenSmoothNormals` : génère des **normales lissées** si le modèle n'en a pas.
- `aiProcess_FlipUVs` : retourne l'axe V des UV (origine OpenGL en bas vs convention des fichiers).
- `aiProcess_JoinIdenticalVertices` : fusionne les sommets dupliqués (allège le maillage).

### Notre conversion (`mesh_loader.cpp`)

`load_from_file` fait :
1. `ReadFile` avec les flags ci-dessus (échoue proprement si fichier illisible).
2. Crée un `Mesh` vide, puis appelle `process_node` sur la racine avec la matrice identité.
3. `process_node` (récursif) : pour chaque mesh du nœud,
   - applique la **transformation cumulée** à chaque sommet (`transform * mVertices[idx]`)
     et aux normales (matrice **inverse-transposée**, ici approximée par transposée + renormalisation) ;
   - lit l'UV (ou (0,0) à défaut) ;
   - empile chaque sommet via `m->push_vertex(...)` ;
   - lit le **chemin de la texture diffuse** du matériau → enregistre un `SubMesh`
     (plage de sommets + chemin) via `m->push_submesh(...)` ;
   - recurse sur les nœuds enfants.
4. Au final, `Mesh::upload()` charge la texture de chaque `SubMesh`, et `draw_textured`
   dessine chaque sous-mesh en liant sa texture (`use_texture = 1`).

### Pourquoi un fichier séparé

`mesh_loader.cpp` est le **seul** à inclure/lier Assimp. Ainsi `mesh.hh`/`mesh.cpp` n'ont
aucune dépendance Assimp, et le binaire **`cartoon` se construit sans Assimp**. Le `Makefile`
ne compile/lie `zelda` que si `pkg-config` détecte Assimp (`sudo pacman -S assimp` sur Arch).

### Limite connue

Les fichiers de **texture** (`*.png`) doivent être présents à côté des modèles. Le modèle
charge sa géométrie même si une texture manque (rendu non texturé pour le sous-mesh concerné).
