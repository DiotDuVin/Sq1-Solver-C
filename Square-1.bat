@echo off
rem Ouvre le visualiseur Square-1 dans le navigateur par defaut (Windows).
rem Le dossier doit etre decompresse (ne pas lancer depuis le fichier .zip).
if not exist "%~dp0gui\index.html" (
  echo Fichier gui\index.html introuvable : decompressez d abord le .zip ^(clic droit ^> Extraire tout^).
  pause
  exit /b 1
)
start "" "%~dp0gui\index.html"
