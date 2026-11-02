#!/usr/bin/env bash
# Installe les commandes :
#   sq1-gui     ouvre l'interface graphique (2D / 3D) dans le navigateur
#   sq1-solver  le solveur en ligne de commande (compilé avec gcc -O2)
#
# Usage : ./install.sh            installer / mettre à jour
#         ./install.sh --uninstall désinstaller
set -euo pipefail

SRC="$(cd "$(dirname "$0")" && pwd)"
BIN="${XDG_BIN_HOME:-$HOME/.local/bin}"
SHARE="${XDG_DATA_HOME:-$HOME/.local/share}"
DEST="$SHARE/sq1-solver"
DESKTOP="$SHARE/applications/sq1-gui.desktop"

if [ "${1:-}" = "--uninstall" ]; then
  rm -rf "$DEST" "$BIN/sq1-gui" "$BIN/sq1-solver" "$DESKTOP"
  echo "Square-1 Solver désinstallé."
  exit 0
fi

mkdir -p "$BIN" "$DEST"

# 1. Copie de l'interface (la commande fonctionne même si le dépôt est déplacé)
rm -rf "$DEST/gui"
cp -r "$SRC/gui" "$DEST/gui"
rm -rf "$DEST/gui/wasm" "$DEST/gui/build.sh"

# 2. Solveur en ligne de commande
if command -v gcc >/dev/null 2>&1; then
  gcc -O2 "$SRC/sq1_solver.c" -o "$BIN/sq1-solver"
  echo "✓ sq1-solver compilé"
else
  echo "! gcc introuvable : sq1-solver n'est pas installé (seule l'interface le sera)"
fi

# 3. Commande sq1-gui
cat > "$BIN/sq1-gui" <<GUI
#!/usr/bin/env bash
# Ouvre le visualiseur Square-1.
#   sq1-gui                        position d'exemple
#   sq1-gui A1B2C3D46E5F7G8H/      résout cette position
#   sq1-gui -s "(1,0)/(-1,-1)/"    anime ce mélange puis sa solution
#   sq1-gui --3d ...               ouvre directement la vue 3D (--2d, --both)
PAGE="$DEST/gui/index.html"
args=()
while [ \$# -gt 0 ]; do
  case "\$1" in
    -h|--help) sed -n '2,6p' "\$0" | sed 's/^# \\{0,1\\}//'; exit 0 ;;
    --2d) args+=("view=2d") ;;
    --3d) args+=("view=3d") ;;
    --both) args+=("view=both") ;;
    -s|--scramble) shift; args+=("scramble=\$(printf '%s' "\${1:-}" | sed 's/(/%28/g; s/)/%29/g; s/,/%2C/g; s#/#%2F#g; s/ //g')") ;;
    *) args+=("pos=\${1//\\//%2F}") ;;
  esac
  shift
done
URL="file://\$PAGE"
[ \${#args[@]} -gt 0 ] && URL="\$URL#\$(IFS='&'; echo "\${args[*]}")"

if [ -n "\${BROWSER:-}" ]; then opener="\$BROWSER"
elif [ "\$(uname)" = "Darwin" ]; then opener=open
elif command -v wslview >/dev/null 2>&1; then opener=wslview
elif command -v xdg-open >/dev/null 2>&1; then opener=xdg-open
else echo "Ouvrez cette adresse dans votre navigateur : \$URL"; exit 0
fi
"\$opener" "\$URL" >/dev/null 2>&1 &
GUI
chmod +x "$BIN/sq1-gui"
echo "✓ sq1-gui installé"

# 4. Raccourci dans le menu des applications (Linux)
if [ "$(uname)" = "Linux" ]; then
  mkdir -p "$(dirname "$DESKTOP")"
  cat > "$DESKTOP" <<DESK
[Desktop Entry]
Type=Application
Name=Square-1 Solver
Comment=Visualiseur 2D/3D du solveur Square-1
Exec=$BIN/sq1-gui
Terminal=false
Categories=Education;Game;
DESK
  echo "✓ raccourci « Square-1 Solver » ajouté au menu des applications"
fi

case ":$PATH:" in
  *":$BIN:"*) ;;
  *) echo
     echo "! $BIN n'est pas dans votre PATH. Ajoutez cette ligne à ~/.bashrc (ou ~/.zshrc) :"
     echo "    export PATH=\"$BIN:\$PATH\""
     echo "  puis ouvrez un nouveau terminal." ;;
esac

echo
echo "Terminé. Lancez :  sq1-gui"
