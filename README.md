# Acid303 - NTS-1 (nutekt-digital)

Compilation :
1. Installe le logue SDK + arm-none-eabi-gcc (voir README du SDK).
2. Copie `platform/nutekt-digital/dummy-osc` vers `platform/nutekt-digital/acid303`.
3. Remplace `osc.cpp`, `manifest.json`, `project.mk` par ceux de ce dossier.
4. `make` -> produit `acid303.ntkdigunit`.
5. Charge-le avec Korg Kontrol Editor ou `logue-cli load -u acid303.ntkdigunit`.

Controles :
- Shape : saw -> carre
- Alt (Shift Shape) : largeur d'impulsion
- Param 1 Slide : glissando entre notes
- Param 2 Drive : saturation (accent)
- Param 3 Sub : sous-octave

Pour le son 303 : filtre NTS-1 en passe-bas, resonance haute, EG filtre court, mode Mono.

## Build automatique

Chaque push lance GitHub Actions. Le fichier `acid303.ntkdigunit` est dans Actions > dernier run > Artifacts.
