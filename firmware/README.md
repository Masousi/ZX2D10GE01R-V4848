<!-- ⚫ AJOUTÉ (24/10/2026) : firmware prêt à flasher, sans avoir à compiler -->
<!-- ⚫ MODIFIÉ (24/10/2026) : un firmware par démo (5 démos), régénérés avec full_refresh, encodeur corrigé et LED -->
# Firmwares précompilés

Chaque fichier est une **image complète** (bootloader + table de partitions + application), à flasher à l'adresse `0x0`.

<!-- ⚫ MODIFIÉ (24/10/2026) : 7 firmwares régénérés (menu principal + 6 démos seules), chacun flashé et vérifié sur la carte -->
| Fichier | Démo | Taille | SHA-256 |
|---|---|---|---|
| `ZX2D10GE01R_demo_all.bin` | **Menu principal : toutes les démos** (choix à l'encodeur, appui long 3 s = retour au menu) | 1 253 712 o | `555d6491fa391b883d984b53eed72b3571d59ec0eb12793d5b37f8a733a6f2d6` |
| `ZX2D10GE01R_demo_widgets.bin` | Widgets (onglets qui défilent seuls toutes les 5 s, navigation à l'encodeur) | 727 552 o | `bb49bd63b73d67427714c0914d2ee3104d2049018bf2b125d9bdc7ade5d5ed1f` |
| `ZX2D10GE01R_demo_keypad_encoder.bin` | Keypad and encoder (formulaire piloté par l'encodeur) | 576 672 o | `82ae136d42403efe02ae077bf14d7fd5935ca6aa902ecef899bbb6b9b7b62d75` |
| `ZX2D10GE01R_demo_benchmark.bin` | Benchmark (scènes animées puis résultat en FPS) | 670 032 o | `8c7823a45fc1aeb3692438271d255cdfcb5a26daabf4ed0800e5ffccd032dc8c` |
| `ZX2D10GE01R_demo_stress.bin` | Stress (création/suppression d'objets en boucle) | 581 696 o | `4a3fbcccef413fd687ebc97d2eff461928d6db9907774af5c9e6431391c3bcee` |
| `ZX2D10GE01R_demo_music.bin` | Music (480×480 carré, lecture automatique) | 976 720 o | `2723cf30f8cbbb854b4d497a41640c28064d2384f7cd91e1b118bf6b33c0e768` |
| `ZX2D10GE01R_demo_led.bin` | Animations LED (choix de l'animation de l'anneau : 7 effets + Random) | 615 008 o | `c8d1ffce809185e79774a957785793c8ff9cb26fe96aa65a7da3d6872841d762` |

<!-- ⚫ MODIFIÉ (24/10/2026) : état réel des fichiers fournis (régénération interrompue) -->
<!-- ⚫ MODIFIÉ (24/10/2026) : régénération complète faite, l'avertissement ci-dessous n'est plus d'actualité (conservé pour l'historique) :
Tous incluent le bouton rotatif fonctionnel et l'animation de l'anneau de LED (13 LED) : suivi de la rotation
(cyan / orange) et flash blanc à l'appui. Luminosité : **80/255** (Benchmark : 255).

> ⚠️ Ces fichiers ont été générés **avant** l'ajout du **chenillard multicolore** d'inactivité et du passage
> de la luminosité par défaut à 100 % : ils ne les contiennent pas encore. Pour en profiter, compiler depuis
> les sources (voir ci-dessous), puis régénérer les `.bin`.
-->
Tous incluent le bouton rotatif fonctionnel et l'animation de l'anneau de LED (13 LED) : suivi de la rotation
(cyan / orange), flash blanc à l'appui et chenillard multicolore après 5 s d'inactivité. Luminosité : **255/255**.

Vérification faite sur la carte pour chaque fichier : flash (hash vérifié), démarrage unique sans erreur, et ligne
attendue dans le journal série (`MENU: Menu principal` ou `MAIN: Demo: <démo>`).

- Cible : ESP32-S3 N16R8 (flash 16 Mo, PSRAM octale 8 Mo), ESP-IDF v5.0.9.

## Flasher

```sh
esptool.py --chip esp32s3 -p COM4 -b 460800 write_flash 0x0 ZX2D10GE01R_demo_widgets.bin
```

Remplacer le nom du fichier par la démo voulue et `COM4` par le port de la carte.
Ou avec l'outil web : https://espressif.github.io/esptool-js/ (adresse `0x0`).

## Régénérer après un changement de démo

1. `idf.py menuconfig` → **ZX2D10 Demo** → *Demo to run* <!-- ⚫ 24/10/2026 : *Main menu (all demos)* → `_all`, sinon `_<démo>` -->
2. `idf.py build`
3. Dans `build/` :
   ```sh
   esptool.py --chip esp32s3 merge_bin -o ../firmware/ZX2D10GE01R_demo_<nom>.bin @flash_args
   ```
