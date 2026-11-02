#!/usr/bin/env python3
"""Génère dist/Square-1.html : toute l'interface (CSS, three.js, solveur
WebAssembly, JS) dans un seul fichier, à télécharger et ouvrir directement.

Usage : python3 gui/bundle.py   (après ./gui/build.sh si sq1_solver.c a changé)
"""
import pathlib, re

GUI = pathlib.Path(__file__).resolve().parent
OUT = GUI.parent / "dist" / "Square-1.html"

html = (GUI / "index.html").read_text(encoding="utf-8")

def inline_css(m):
    css = (GUI / m.group(1)).read_text(encoding="utf-8")
    return "<style>\n" + css + "\n</style>"

def inline_js(m):
    js = (GUI / m.group(1)).read_text(encoding="utf-8")
    return "<script>\n" + js.replace("</script", "<\\/script") + "\n</script>"

html = re.sub(r'<link rel="stylesheet" href="([^"]+)">', inline_css, html)
html = re.sub(r'<script src="([^"]+)"></script>', inline_js, html)
assert 'src="' not in html and 'href="style' not in html, "ressource non intégrée"

OUT.parent.mkdir(exist_ok=True)
OUT.write_text(html, encoding="utf-8")
print(f"OK : {OUT.relative_to(GUI.parent)} ({OUT.stat().st_size // 1024} Ko)")
