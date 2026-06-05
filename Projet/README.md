# Rendu Cartoon Temps Réel — POGL

Rendu **cartoon (cel shading) temps réel** en OpenGL 4.5, avec contours, ombrage
en hachures, ombres portées, **cycle jour/nuit** et styles commutables en direct.

Le projet produit **deux binaires** (mêmes shaders et même pipeline cartoon) :
- **`cartoon`** — un **désert façon Lucky Luke** : buttes géantes (mesas), herbe et
  cailloux procéduraux sur un grand sol sableux.
- **`zelda`** — un **visualiseur de modèles externes** (Wind Waker : Toon Link + le
  bateau *King of Red Lions*) chargés via **Assimp**.

> Projet de Programmation OpenGL (POGL). Sujet : « Rendu cartoon temps réel ».

**Auteurs (binôme) :** Léo LI · _(binôme à compléter)_

---

## Compilation & exécution

```bash
cd Projet
make            # construit LES DEUX binaires : cartoon et zelda
./cartoon       # le désert
./zelda         # le visualiseur Zelda (Toon Link + bateau)
make clean      # nettoyer les objets + binaires
```

**Dépendances :** OpenGL 4.5, GLEW, FreeGLUT. **Assimp requis uniquement pour `zelda`**
(chargement de modèles) — `cartoon` se construit sans. Si Assimp est absent, `make` bâtit
`cartoon` et saute `zelda` (message). Sur Arch : `sudo pacman -S assimp`.

## Contrôles

| Touche | Action |
|---|---|
| Flèches | orbite caméra (azimut / élévation) |
| A / E | zoom (rapproche / éloigne) |
| ZQSD | déplacement horizontal au sol |
| Espace / C | monter / descendre |
| + / − | nombre de bandes du cel shading |
| H | hachures de croquis on/off |
| 1 – 4 | presets de style (4 = Phong, pour la comparaison) |
| T | vitesse du cycle jour/nuit (×1 → ×8 → ×30) |
| P | pause animation · Échap | quitter |

**Styles :** `1` BD cartoon · `2` Croquis encre · `3` Pop comics · `4` Phong (comparaison).

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

Techniques mises en œuvre (détail dans les `PHASE_*.md`) :

- **Cel shading** : `floor(dot(n,l)·N)/N`, spéculaire seuillé, lumière directionnelle.
- **Contour post-process** : détection de bords sur G-buffer (profondeur en dérivée
  seconde → robuste aux plans rasants ; normale → arêtes vives), trait à épaisseur et
  couleur variables selon la profondeur.
- **Cycle jour/nuit** : le soleil tourne (`light = dir(cosθ, sinθ, .35)`) ; sa hauteur
  fait évoluer ciel, lumière, ambiante et ombres (= l'animation temps réel de la scène).
- **Ombres** : projection planaire sur le sol (`lw=0`), **clippées aux bornes du terrain**
  (`discard`), couleur dérivée du sol éclairé (suit le jour/nuit), coupées la nuit.
- **Hachures** : trame de croquis en espace écran, densité ∝ obscurité.
- **Presets de style** : table `Style` factorisant tout le « look », bascule clavier ;
  un preset Phong (64 bandes, sans contour ni hachures) sert de comparaison.
- **Géométrie procédurale** (desert) : buttes (`make_butte` = rocher bruité tronqué →
  mesa à sommet plat), rochers (`make_rock`), herbe (`make_grass`), sol (`make_plane`).
  Scène = liste d'instances `{mesh, model, couleur}` : une boucle ombres, une boucle cel.
- **Chargement de modèles** (zelda) : `Mesh::load_from_file` via **Assimp** (OBJ/glTF/DAE)
  → remplit le même VAO + des `SubMesh` texturés ; `draw_textured` lie la texture par
  sous-mesh. Les deux apps partagent caméra, shaders et pipeline (un seul `mesh.cpp`).

## Arborescence

```
Projet/
├── Makefile
├── src/
│   ├── main.cpp                 app DÉSERT (→ cartoon) : scène, jour/nuit, presets, clavier
│   ├── main_zelda.cpp           app ZELDA (→ zelda) : charge Toon Link + bateau
│   ├── mesh_loader.cpp          loader Assimp (lié à zelda SEUL — cartoon n'en dépend pas)
│   ├── matrix4.{hh,cpp}         maths : lookat, frustum, translate/rotate/scale, shadow   ┐
│   ├── camera.{hh,cpp}          caméra orbitale + déplacement fly                          │ partagés
│   ├── program.{hh,cpp}         compilation/link des shaders                               │ par les
│   ├── mesh.{hh,cpp}            meshes procéduraux (+ push_vertex pour le loader)           │ 2 apps
│   └── texture.{hh,cpp}         chargement de texture PNG (stb_image)                      ┘
├── shaders/   cel / shadow / post (.glsl) — partagés par les 2 binaires
├── assets/    Toon Link (obj+mtl), red_lion (gltf) — modèles chargés par zelda
├── lib/       stb_image.h (chargement PNG, en-tête tiers)
└── *.md       CONTEXT, PHASES, PHASE_1..6, CODE  documentation
```

> Le code est volontairement **sans commentaires** — le rôle de chaque fichier et de
> chaque fonction (et le détail du chargement **Assimp**) est documenté dans **`CODE.md`**.

## Notes

- **Deux binaires, code partagé** : `cartoon` et `zelda` lient les mêmes objets
  (caméra, shaders, `mesh.cpp`) ; seul le `main` diffère. Le shader d'ombre est
  rétro-compatible (clip du terrain seulement si `ground_half > 0`).
- ⚠️ **Textures des modèles Zelda manquantes du dépôt** : `.gitignore` exclut `*.png`,
  donc les textures de Toon Link (`clothes.png`…) et `eau_windwaker.png` ne sont **pas
  commitées**. Le modèle charge sa géométrie mais sans texture tant qu'elles ne sont pas
  ajoutées (`git add -f` ou exception `assets/` dans `.gitignore`).
- Les shaders `.glsl` sont **lus à l'exécution** : pas besoin de recompiler, juste relancer.
- `GL error 1280` au démarrage : bénin (`glewInit()` en profil core).
- Capture d'écran auto indisponible sous Wayland/Hyprland → vérification visuelle manuelle.
