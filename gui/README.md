# Interface graphique (visualiseur)

Page web qui rejoue pas à pas les solutions trouvées par `../sq1_solver.c`.
Le solveur C est compilé tel quel en **WebAssembly** et s'exécute dans le navigateur :
c'est exactement le même code que la version en ligne de commande.

## Utilisation

Ouvrir `index.html` dans un navigateur (double-clic, aucun serveur nécessaire).

- **Position** : même format que le programme (`A1B2C3D45E6F7G8H-`), puis *Résoudre*.
- **Position aléatoire** : position bien mélangée (40 twists aléatoires), puis résolution.
- **Mélange en notation** : `(1,0)/(-1,-1)/…` appliqué à l'état résolu. La page anime
  le mélange (en gris) puis la solution, avec la phase 1 en violet et la phase 2 en vert.
  *Générer* produit un mélange aléatoire valide de 12 twists.
- Lecture : ▶ / ⏸, pas à pas, curseur, vitesse, clic sur un mouvement.
  Raccourcis clavier : `←` `→`, `Espace`, `Origine`, `Fin`.

Trois modes d'affichage : **2D**, **3D** ou **2D + 3D** (le choix est mémorisé).
Dans la vue 3D, glisser pour faire tourner le puzzle (y compris pour voir le dessous),
molette pour zoomer, double-clic pour revenir à la vue initiale. Les rotations et les
twists y sont animés en continu.

Les vues 2D montrent la couche du haut vue de dessus, la couche médiane (carré ou kite)
et la couche du bas vue *de dessus, par transparence*, pour que la moitié qui
tourne lors d'un twist `/` soit à droite du trait pointillé sur les deux vues.

## Recompiler le WebAssembly

Seuls `clang` et `lld` sont nécessaires (pas d'Emscripten) :

```bash
sudo apt install clang lld
./build.sh
```

`build.sh` produit `sq1.wasm` et `sq1-wasm.js`, qui contient le même binaire encodé en
base64 pour que la page fonctionne aussi en `file://`. Il faut relancer le script
après chaque modification de `sq1_solver.c`.

## Organisation

| Fichier | Rôle |
|---|---|
| `wasm/sq1_wasm.c` | Inclut `../../sq1_solver.c` sans son `main`, redirige les `printf`, et expose `sq1_init`, `sq1_solve` (résultat en JSON avec l'état après chaque étape). |
| `wasm/libc/*.h` | En-têtes minimaux, pour compiler sans libc. |
| `app.js` | Dessin SVG des pièces, animations, simulation des mouvements pour les mélanges. |
| `view3d.js` | Vue 3D : les polygones de la vue 2D extrudés avec three.js, autocollants et caméra orbitale. |
| `vendor/three.min.js` | three.js r149 (licence MIT, `vendor/three.LICENSE`), inclus pour fonctionner hors ligne et en `file://`. |
| `index.html`, `style.css` | Page et mise en forme. |

Côté `sq1_solver.c`, seules deux petites modifications ont été faites : la dernière
solution est copiée dans `solutionMoves` / `solutionLen` (dans `print_solution`), et
`main` est entouré d'un `#ifndef SQ1_NO_MAIN`. Le programme en ligne de commande se
compile et fonctionne comme avant.
