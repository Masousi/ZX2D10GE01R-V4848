<!-- ⚫ AJOUTÉ (24/10/2026) : firmware prêt à flasher, sans avoir à compiler -->
<!-- ⚫ MODIFIÉ (24/10/2026) : un firmware par démo (5 démos), régénérés avec full_refresh, encodeur corrigé et LED -->
# Firmwares précompilés

Chaque fichier est une **image complète** (bootloader + table de partitions + application), à flasher à l'adresse `0x0`.

| Fichier | Démo | Taille | SHA-256 |
|---|---|---|---|
| `ZX2D10GE01R_demo_widgets.bin` | Widgets (onglets qui défilent seuls toutes les 5 s, navigation à l'encodeur) | 717 568 o | `fff090b8c2371572650c76b9ebe704d920324d72779bf9dfcff2d59bf33db4ac` |
| `ZX2D10GE01R_demo_keypad_encoder.bin` | Keypad and encoder (formulaire piloté par l'encodeur) | 566 672 o | `198cc7f70ec9f120996a0afa46dc744b737ee63873f18c777235974891e4aebe` |
| `ZX2D10GE01R_demo_benchmark.bin` | Benchmark (scènes animées puis résultat en FPS) | 659 984 o | `f3f464e5df9f299a8eb59546a92c6a3166f434228b827f9e696fd7af915997c0` |
| `ZX2D10GE01R_demo_stress.bin` | Stress (création/suppression d'objets en boucle) | 571 696 o | `3167c4fb916862f46a6058070ec101f73822e072d418bf40806dd9947b7202b4` |
| `ZX2D10GE01R_demo_music.bin` | Music (480×480 carré, lecture automatique) | 966 688 o | `9287b146426ebe80891fb181b43a7285dbff68fc05c300e07c4b6b1d7df46d33` |

<!-- ⚫ MODIFIÉ (24/10/2026) : état réel des fichiers fournis (régénération interrompue) -->
Tous incluent le bouton rotatif fonctionnel et l'animation de l'anneau de LED (13 LED) : suivi de la rotation
(cyan / orange) et flash blanc à l'appui. Luminosité : **80/255** (Benchmark : 255).

> ⚠️ Ces fichiers ont été générés **avant** l'ajout du **chenillard multicolore** d'inactivité et du passage
> de la luminosité par défaut à 100 % : ils ne les contiennent pas encore. Pour en profiter, compiler depuis
> les sources (voir ci-dessous), puis régénérer les `.bin`.

- Cible : ESP32-S3 N16R8 (flash 16 Mo, PSRAM octale 8 Mo), ESP-IDF v5.0.9.

## Flasher

```sh
esptool.py --chip esp32s3 -p COM4 -b 460800 write_flash 0x0 ZX2D10GE01R_demo_widgets.bin
```

Remplacer le nom du fichier par la démo voulue et `COM4` par le port de la carte.
Ou avec l'outil web : https://espressif.github.io/esptool-js/ (adresse `0x0`).

## Régénérer après un changement de démo

1. `idf.py menuconfig` → **ZX2D10 Demo** → *Demo to run*
2. `idf.py build`
3. Dans `build/` :
   ```sh
   esptool.py --chip esp32s3 merge_bin -o ../firmware/ZX2D10GE01R_demo_<nom>.bin @flash_args
   ```
