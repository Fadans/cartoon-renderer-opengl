# Rendu Cartoon Temps Réel — POGL

Rendu **cartoon (cel shading) temps réel** en OpenGL 4.5, avec contours par
détection de bords, ombrage en hachures, ombres portées et styles commutables en direct.

Le projet produit **trois binaires** (mêmes shaders, même pipeline cartoon) :
- **`desert`** — un **désert façon Lucky Luke** : buttes géantes (mesas), herbe et
  cailloux procéduraux sur un grand sol sableux, avec ombres portées.
- **`meshes`** — une **scène « mesh simple »** : trois primitives géométriques
  procédurales (sphère, cylindre, tore) qui tournent sur un sol, avec ombres portées.
- **`pont`** — un **pont japonais** : un vrai modèle **glTF** externe chargé via **Assimp**,
  auto-cadré et colorié par matériau.

> Projet de Programmation OpenGL (POGL). Sujet : « Rendu cartoon temps réel ».

**Auteurs (binôme) :** Léo LI · _(binôme à compléter)_

---

## Compilation & exécution

```bash
cd Projet
make            # construit les trois binaires : desert, meshes, pont
./desert        # le désert procédural
./meshes        # les 3 primitives sur un sol
./pont          # le pont japonais (modèle glTF)
make clean      # nettoyer les objets + binaires
```

**Dépendances :** OpenGL 4.5, GLEW, FreeGLUT. **Assimp requis uniquement pour `pont`**
(chargement du modèle) — `desert` et `meshes` se construisent sans. Si Assimp est absent,
`make` bâtit `desert` et `meshes` et saute `pont` (message). Sur Arch : `sudo pacman -S assimp`.

## Contrôles

| Touche | Action |
|---|---|
| Flèches | orbite caméra (azimut / élévation) |
| A / E | zoom (rapproche / éloigne) |
| ZQSD | déplacement horizontal au sol |
| Espace / C | monter / descendre |
| + / − | nombre de bandes du cel shading |
| H | hachures de croquis on/off |
| 1 – 3 | presets de style (3 = Phong, pour la comparaison) |
| P | pause animation (Échap | quitter) |

**Styles :** `1` Croquis encre (hachures) · `2` Pop comics (aplats) · `3` Phong (comparaison).
Par défaut : `meshes` démarre en **style 2** (pop comics), `pont` en **style 1** (hachures).
Sur `pont`, **Espace** fait tourner/arrête le modèle (au lieu de la pause).

---

## Aperçu technique

Pipeline en **2 passes** (FBO + détection de bords en post-process) :

```
Passe 1 → FBO {couleur RGBA8, normales RGB16F, profondeur D24}
          cel shading (N paliers + spéculaire binaire + hachures écran)
          + ombres projetées planaires (matrice d'ombre, polygon offset)
Passe 2 → écran : triangle plein écran (VAO vide, gl_VertexID)
          fond gradient + contour (Laplacien de profondeur ∪ saut de normale),
          épaisseur/couleur du trait variables avec la profondeur
```

Techniques mises en œuvre (détail dans les `PHASE_*.md` et `GUIDE_COMPLET.md`) :

- **Cel shading** : `floor(dot(n,l)·N)/N`, spéculaire seuillé, **lumière directionnelle fixe**.
- **Contour post-process** : détection de bords sur G-buffer (profondeur en dérivée
  seconde → robuste aux plans rasants ; normale → arêtes vives), trait à épaisseur et
  couleur variables selon la profondeur.
- **Ombres** : projection planaire sur le sol (`lw=0`), **clippées aux bornes du terrain**
  (`discard`), couleur dérivée du sol éclairé. Présentes dans `desert` et `meshes`.
- **Hachures** : trame de croquis en espace écran, densité ∝ obscurité ; seuil de
  déclenchement réglable par scène (`hatch_thr`).
- **Presets de style** : table `Style` partagée (`style.hh`) factorisant tout le « look »,
  bascule clavier ; un preset Phong (64 bandes, sans contour ni hachures) sert de comparaison.
- **Géométrie procédurale** : primitives (`make_sphere`, `make_cylinder`, `make_torus`),
  et pour le désert buttes (`make_butte` = rocher bruité tronqué → mesa à sommet plat),
  rochers (`make_rock`), herbe (`make_grass`), sol (`make_plane`). Une scène = liste
  d'instances `{mesh, model, couleur}` : une boucle ombres, une boucle cel.
- **Chargement de modèle** (`pont`) : `Mesh::load_from_file` via **Assimp** (OBJ/glTF/DAE)
  → remplit le même VAO + des `SubMesh` ; couleur diffuse lue par matériau (corrigée en sRGB),
  `draw_colored` applique la couleur de chaque sous-mesh. Auto-cadrage via `Mesh::bounds()`.
  Normales du modèle redressées vers la caméra (`two_sided`) car l'export est incohérent.

## Arborescence

```
Projet/
├── Makefile
├── src/
│   ├── main_desert.cpp          app DÉSERT  (→ desert)  : buttes/herbe/cailloux, presets
│   ├── main_meshes.cpp          app MESHES  (→ meshes)  : sphère/cylindre/tore + sol/ombres
│   ├── main_pont.cpp            app PONT    (→ pont)    : charge le modèle glTF (Assimp)
│   ├── mesh_loader.cpp          loader Assimp (lié à pont SEUL — desert/meshes n'en dépendent pas)
│   ├── style.hh                 table des 3 presets de style (partagée)
│   ├── matrix4.{hh,cpp}         maths : lookat, frustum, translate/rotate/scale, shadow   ┐
│   ├── camera.{hh,cpp}          caméra orbitale + déplacement fly                          │ partagés
│   ├── program.{hh,cpp}         compilation/link des shaders                               │ par les
│   ├── mesh.{hh,cpp}            meshes procéduraux + API de chargement (bounds/draw_colored)│ 3 apps
│   └── texture.{hh,cpp}         chargement de texture PNG (stb_image)                      ┘
├── shaders/   cel / shadow / post (.glsl) — partagés par les 3 binaires
├── assets/    maquette/ (pont japonais, scene.gltf + scene.bin)
├── lib/       stb_image.h (chargement PNG, en-tête tiers)
└── *.md       GUIDE_COMPLET, CODE, CONTEXT, PHASES, PHASE_1..6, SOUTENANCE
```

> **`GUIDE_COMPLET.md`** = explication de A à Z (notions de base → techniques → phases).
> Le code est volontairement **sans commentaires** ; le rôle de chaque fichier/fonction
> (et le détail **Assimp**) est dans **`CODE.md`**. Script oral → **`SOUTENANCE.md`**.

## Notes

- **Trois binaires, code partagé** : `desert`, `meshes` et `pont` lient les mêmes objets
  (caméra, shaders, `mesh.cpp`, `style.hh`) ; seul le `main` diffère. Le shader d'ombre est
  rétro-compatible (clip du terrain seulement si `ground_half > 0`).
- Les shaders `.glsl` sont **lus à l'exécution** : pas besoin de recompiler, juste relancer.
- `GL error 1280` au démarrage : bénin (`glewInit()` en profil core).
- Capture d'écran auto indisponible sous Wayland/Hyprland → vérification visuelle manuelle.

## Crédits

Le modèle 3D du pont japonais (`Projet/assets/maquette/`) est
[« Japanese Bridge Garden »](https://sketchfab.com/3d-models/japanese-bridge-garden-d122e17593eb4012913cde927486d15a)
par [kristenlee](https://sketchfab.com/kristenlee), sous licence
[CC-BY-4.0](http://creativecommons.org/licenses/by/4.0/).

Le chargeur d'images utilise [stb_image](https://github.com/nothings/stb) (domaine public).
