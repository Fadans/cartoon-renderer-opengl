# Rendu Cartoon Temps Réel — POGL

Rendu **cartoon (cel shading) temps réel** en OpenGL 4.5, avec contours, ombrage
en hachures, ombres portées et styles commutables en direct. La scène de démonstration
est une **« casse de Pandora »** (clin d'œil à *Borderlands*) : cuve, barils explosifs,
caisses, pneu, cône et panneau tournant, posés sur un sol désertique — tout en aplats
de couleur soulignés d'un gros trait d'encre.

> Projet de Programmation OpenGL (POGL). Sujet : « Rendu cartoon temps réel ».

**Auteurs (binôme) :** Léo LI · _(binôme à compléter)_

---

## Compilation & exécution

```bash
cd Projet
make            # g++, C++11, profil OpenGL core 4.5
./cartoon
make clean      # nettoyer les objets + binaire
```

**Dépendances :** OpenGL 4.5, GLEW, FreeGLUT (`-lGL -lGLEW -lglut -lpthread`).

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
- **Ombres** : projection planaire sur le sol (matrice d'ombre, `lw=0` = soleil).
- **Hachures** : trame de croquis en espace écran, densité ∝ obscurité.
- **Presets de style** : table `Style` factorisant tout le « look », bascule clavier ;
  un preset Phong (64 bandes, sans contour ni hachures) sert de comparaison.
- **Scène** : liste d'instances `{mesh, model, couleur}` composée à partir de meshes
  procéduraux (cube/cône via `make_box`/`make_cone`, **rochers facettés** via `make_rock`
  = sphère déformée par bruit à normales plates), posés au sol ; une boucle pour les
  ombres, une pour le rendu cel. Un panneau se balance (animation temporelle).

## Arborescence

```
Projet/
├── Makefile
├── src/
│   ├── main.cpp                 point d'entrée, boucle de rendu, presets, clavier
│   ├── matrix4.{hh,cpp}         maths : lookat, frustum, translate/rotate/scale, shadow
│   ├── mesh.{hh,cpp}            géométrie procédurale (sphère, cylindre, tore, cube, cône, rocher, sol)
│   ├── camera.{hh,cpp}          caméra orbitale + déplacement fly
│   ├── program.{hh,cpp}         compilation/link des shaders
│   └── texture.{hh,cpp}         loader de texture (dormant — voir Notes)
├── shaders/
│   ├── cel_{vertex,fragment}.glsl       cel shading + hachures (passe 1)
│   ├── shadow_{vertex,fragment}.glsl    ombres projetées planaires
│   └── post_{vertex,fragment}.glsl      fond + contour (passe 2)
├── lib/  image*.{hh,cpp}        chargement TGA (TIFO) — utilisé par texture.* (dormant)
└── *.md  CONTEXT, PHASES, PHASE_1..5  documentation détaillée par phase
```

## Notes

- **Aplats de couleur** : le rendu utilise des couleurs unies par objet. La branche
  `use_texture` (shader + `texture.*` + `lib/`) est **conservée mais inactive**, prête
  pour d'éventuelles textures faites maison.
- Les shaders `.glsl` sont **lus à l'exécution** : modifier un shader ne nécessite pas
  de recompiler, juste relancer `./cartoon`.
- `GL error 1280` au démarrage : bénin, laissé par `glewInit()` en profil core.
- Capture d'écran automatique indisponible sous Wayland/Hyprland → vérification visuelle
  manuelle.
