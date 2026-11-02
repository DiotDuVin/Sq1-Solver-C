#!/usr/bin/env bash
# Compile le solveur en WebAssembly (sans Emscripten : clang + wasm-ld suffisent)
# puis génère sq1-wasm.js (le .wasm encodé en base64) pour que index.html
# fonctionne même ouvert directement depuis le disque (file://).
#
# Dépendances (Debian/Ubuntu) : sudo apt install clang lld
set -euo pipefail
cd "$(dirname "$0")"

clang --target=wasm32 -O3 -ffreestanding -nostdlib \
      -Iwasm/libc -Wall -Wno-char-subscripts -Wno-unused-function \
      -Wl,--no-entry -Wl,-z,stack-size=1048576 -Wl,--strip-all \
      -o sq1.wasm wasm/sq1_wasm.c

{
  printf '// Fichier généré par build.sh — ne pas modifier à la main.\n'
  printf 'window.SQ1_WASM_BASE64 = "'
  base64 -w0 sq1.wasm
  printf '";\n'
} > sq1-wasm.js

echo "OK : sq1.wasm ($(stat -c %s sq1.wasm) octets) et sq1-wasm.js générés."
