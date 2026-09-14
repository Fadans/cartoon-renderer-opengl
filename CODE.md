# Documentation du code

> Le code source est volontairement **sans commentaires** ; les explications sont
> regroupées ici. Vue d'ensemble du rendu → `CONTEXT.md` ; détail par phase →
> `PHASE_*.md`. Ce fichier décrit **le rôle de chaque fichier et de ses fonctions**.

## Organisation générale

Le projet produit **trois binaires** qui partagent la majorité du code :

| Binaire | `main` | Scène |
|---|---|---|
| `desert` | `src/main_desert.cpp` | désert procédural (buttes, herbe, cailloux) + sol/ombres |
| `meshes` | `src/main_meshes.cpp` | 3 primitives (sphère/cylindre/tore) tournant sur un sol |
| `pont`   | `src/main_pont.cpp`   | modèle glTF externe (pont japonais) chargé via Assimp |

Fichiers **partagés** : `matrix4`, `camera`, `program`, `mesh`, `texture`, `style.hh` + les
shaders. `mesh_loader.cpp` (Assimp) n'est lié qu'à `pont`. Pipeline de rendu en **2 passes** :
passe 1 = cel shading + ombres → FBO ; passe 2 = fond + contour en post-process.

`style.hh` regroupe la **table des 3 presets de style** (Croquis encre / Pop comics / Phong)
partagée par les trois binaires.

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
- `bind(unit)` : lie la texture à une unité (`GL_TEXTURE0`…). `valid()` : chargement réussi.

## `mesh.hh / .cpp` — géométrie

`class Mesh` = un VAO + 3 VBO (positions, normales, UV) + une liste de `SubMesh`
(plages de sommets texturées, pour les modèles chargés).

**Générateurs procéduraux** (renvoient un `Mesh*`) — détail de la **construction** en §2.9
de `GUIDE_COMPLET.md` :
- `make_sphere(rings,sectors,radius)` : **sphère UV** par paramétrage `(φ,θ)` — un quad par
  cellule lat/long, deux triangles, normale = position normalisée.
- `make_cylinder(n,radius,height,with_caps)` : **cylindre** — `n` quads sur le flanc (normale
  radiale) + deux disques en éventail pour les capuchons haut/bas (normale ±Y).
- `make_torus(rings,sectors,R,r)` : **tore** par paramétrage `(u,v)` autour du grand rayon `R`
  et du tube `r` ; normale analytique `(cos v cos u, sin v, cos v sin u)`.
- `make_plane(half, y)` : grand quad horizontal (le sol).
- `make_rock(rings,sectors,radius,jitter,seed)` : rocher facetté = sphère basse résolution
  déformée par un bruit, **normales plates** (aspect low-poly anguleux).
- `make_butte(rings,sectors,radius,height,jitter,seed)` : **mesa** = un `make_rock` dont le
  Y est étiré puis clampé → sommet et base plats, flancs rocheux.
- `make_grass(blades,height,spread,seed)` : touffe de brins (triangles fins double face).
- (Un sous-helper interne `rock_noise` fournit le bruit lisse pour rock et butte.)

**Méthodes d'instance** :
- `upload()` : envoie les VBO au GPU + charge la texture de chaque `SubMesh`.
- `draw()` : dessine tout le mesh (`glDrawArrays`).
- `draw_textured(ul_use_tex)` : dessine **sous-mesh par sous-mesh**, en liant la texture de chacun.
- `draw_colored(ul_use_tex, ul_obj_color)` : dessine sous-mesh par sous-mesh en appliquant la
  **couleur diffuse** lue par matériau (aplat cartoon) — utilisé pour le modèle glTF du pont.
- `bounds(mn, mx)` : **boîte englobante** locale (sur les positions) → sert à **auto-centrer et
  mettre à l'échelle** un modèle chargé (cf. `main_pont.cpp`).
- `push_vertex(...)`, `push_submesh(...)` : API d'ajout utilisée par le loader de modèles
  (`push_submesh` existe avec et sans couleur de matériau).
- `vertex_count()` : nombre de sommets.
- `load_from_file(path)` : **implémentée dans `mesh_loader.cpp`** (voir ci-dessous).

## `mesh_loader.cpp` — chargement de modèles (Assimp) — voir section dédiée

- `Mesh::load_from_file(path)` : charge un modèle externe (OBJ/glTF/DAE) en un `Mesh`.
- `process_node(...)` (interne) : parcours récursif de la hiérarchie du modèle.

## `style.hh` — presets de style (partagé)

`struct Style` + table `g_styles[]` des **3 presets** (Croquis encre = hachures · Pop comics =
aplats · Phong = comparaison). Chaque style fixe : nombre de bandes, hachures on/off, ambiant,
seuils/force spéculaire, couleur et épaisseur du contour (près/loin), et fond (dégradé).
Inclus par les trois `main`, qui gardent chacun leur `apply_style(i)` (recopie bandes/hachures
dans les globals → `+/-` et `H` affinent par-dessus).

## `main_desert.cpp` — application désert (`desert`)

- `read_file(path)` : lit un fichier shader en `std::string`.
- `initglut/initglew/initGL` : contexte OpenGL 4.5 core, états (depth test, culling…).
- `initshaders()` : compile les 3 programmes (cel, post, shadow) et récupère les *uniform locations*.
- `init_fbo(w,h)` : (re)crée le FBO et ses cibles (couleur, normales, profondeur) à la taille fenêtre.
- `initobjects()` : génère les meshes de la scène (buttes, herbe, cailloux, sol).
- `initPOV()` : matrice de projection (frustum). `timer()` : redessine quand l'animation est active.
- `apply_style(i)` : applique un preset de style (bandes, hachures, couleurs…).
- `draw_cel(mesh, model)` / `draw_shadow(mesh, mvp, world)` : envoient les matrices et dessinent.
- `display()` : **cœur du rendu** — **lumière directionnelle fixe** (`LIGHT_DIR`), construit la
  **liste d'instances** de la scène, fait la passe 1 (sol + objets cel + ombres → FBO) puis la
  passe 2 (fond + contour → écran).
- `reshape(w,h)` : viewport + recrée le FBO. `keyboard(...)` : presets (`1-3`), hachures, pause.
- `main()` : enchaîne les init, règle la caméra, lance `glutMainLoop`.

> Le **cycle jour/nuit** (soleil tournant) a été retiré : l'éclairage est désormais une
> **direction fixe**, et le fond vient du preset de style.

## `main_meshes.cpp` — application primitives (`meshes`)

Même squelette que `main_desert.cpp` :
- `initobjects()` génère un **sol** (`make_plane`) + une **sphère**, un **cylindre** et un
  **tore** procéduraux.
- `display()` fait tourner les trois primitives, projette leurs **ombres** sur le sol (même
  matrice d'ombre que le désert), puis applique cel + contour. Démarre en **style 2** (pop comics).

## `main_pont.cpp` — application pont japonais (`pont`)

Même squelette, mais charge un **modèle externe** :
- `initobjects()` appelle `Mesh::load_from_file("assets/maquette/scene.gltf")`, puis utilise
  `Mesh::bounds()` pour **auto-centrer et mettre le modèle à l'échelle** (tient dans une boîte
  de ~3 unités quelle que soit l'échelle native).
- `display()` fait lentement tourner le modèle et le dessine avec `draw_colored` (couleur par
  matériau). Démarre en **style 1** (hachures). Réglages spécifiques passés aux shaders :
  `hatch_thr` (réduit la couverture des hachures) et `two_sided` (redresse les normales
  incohérentes de l'export — voir §Assimp). Touche **Espace** = rotation on/off.

## Shaders (`shaders/*.glsl`)

- `cel_vertex` / `cel_fragment` : **cel shading** (diffus quantifié en N bandes + spéculaire
  seuillé) ; écrit la couleur **et** la normale (G-buffer) ; ajoute les **hachures** d'écran.
  Deux uniforms de réglage : `hatch_thr` (multiplie les seuils de hachures — `<1` = moins de
  hachures, utilisé par le pont) et `two_sided` (retourne la normale vers la caméra si elle
  pointe à l'envers — pour les modèles aux normales incohérentes).
- `shadow_vertex` / `shadow_fragment` : **ombre projetée** ; calcule la position monde au sol
  et **clippe** l'ombre aux bornes du terrain (si `ground_half > 0`).
- `post_vertex` / `post_fragment` : passe plein écran ; **fond gradient** + **détection de
  contour** (Laplacien de profondeur ∪ saut de normale), trait à épaisseur/couleur variables.

---

## Assimp — chargement de modèles externes (détaillé)

**Assimp** (*Open Asset Import Library*) est une bibliothèque qui lit des dizaines de formats
3D (OBJ, glTF, DAE/Collada, FBX…) et les expose sous une **structure commune** en mémoire.
On l'utilise dans `pont` pour charger le modèle **glTF** du pont japonais, sans écrire un
parseur par format. C'est exactement le même loader qu'il s'agisse d'OBJ ou de glTF :
**aucune brique spécifique au glTF**, Assimp s'en charge.

### Ce qu'Assimp renvoie

`importer.ReadFile(path, flags)` renvoie un `aiScene*` :
- `scene->mRootNode` : la **racine** d'un arbre de nœuds (`aiNode`). Chaque nœud a une
  **transformation locale** (`mTransformation`) et des enfants → on accumule les transformations
  du parent vers l'enfant.
- `scene->mMeshes[]` : les **maillages** (`aiMesh`) : sommets (`mVertices`), normales
  (`mNormals`), coordonnées de texture (`mTextureCoords[0]`), faces (`mFaces`, indices).
- `scene->mMaterials[]` : les **matériaux** ; on y lit la **texture diffuse**
  (`GetTexture(aiTextureType_DIFFUSE, …)`) **et la couleur diffuse** (`AI_MATKEY_COLOR_DIFFUSE`)
  pour les modèles sans texture (cas du pont — aplats de couleur par matériau).

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
   - lit le **chemin de la texture diffuse** *et* la **couleur diffuse** du matériau → enregistre
     un `SubMesh` (plage de sommets + chemin + couleur) via `m->push_submesh(...)` ;
   - recurse sur les nœuds enfants.
4. Au final, `Mesh::upload()` charge la texture de chaque `SubMesh` ; `draw_textured` lie la
   texture (`use_texture = 1`), tandis que `draw_colored` applique la **couleur de matériau**
   (cas du pont, sans texture).

### Couleur diffuse & espace colorimétrique

Les couleurs glTF (`baseColorFactor` / `diffuseFactor`) sont en espace **linéaire** et souvent
très basses (un bois à `0.06`…). Affichées brutes, elles paraissent presque noires. On les
**encode donc en sRGB approché** (`pow(c, 1/2.2)`) à la lecture, sinon le modèle est trop sombre.

### Normales incohérentes (`two_sided`)

Le modèle du pont est un export **double-face** aux normales incohérentes : beaucoup de faces
visibles ont une normale qui pointe à l'opposé de la caméra → elles seraient éclairées à 0
(sombres et entièrement hachurées). Le shader cel, via l'uniform `two_sided`, **retourne la
normale vers le spectateur** quand elle pointe à l'envers — ce qui corrige éclairage et hachures.
Activé seulement pour le pont.

### Pourquoi un fichier séparé

`mesh_loader.cpp` est le **seul** à inclure/lier Assimp. Ainsi `mesh.hh`/`mesh.cpp` n'ont
aucune dépendance Assimp, et les binaires **`desert` et `meshes` se construisent sans Assimp**.
Le `Makefile` ne compile/lie `pont` que si `pkg-config` détecte Assimp (`sudo pacman -S assimp`).
