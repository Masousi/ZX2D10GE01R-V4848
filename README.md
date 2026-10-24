# ZX2D10GE01R
ZX2D10GE01R base on https://github.com/espressif/esp-idf/tree/release/v5.0

<!-- ⚫ AJOUTÉ (24/10/2026) : usage du projet, démos et pièges de configuration -->
## Matériel / environnement

- ESP32-S3 N16R8 : flash 16 Mo, PSRAM **octale** 8 Mo, écran RGB 480×480 (frame buffer en PSRAM, `fb_in_psram`) + encodeur MT8901.
- ESP-IDF **v5.0.9**.

## Démos LVGL

La démo lancée au démarrage se choisit dans `idf.py menuconfig` → **ZX2D10 Demo** → *Demo to run*
(Widgets, Keypad & encoder, Benchmark, Stress, Music). Seules les démos activées dans
*Component config → LVGL → Demos* sont proposées.

Un firmware prêt à flasher est fourni dans [`firmware/`](firmware/README.md) (démo Music).

## ⚠️ Configuration indispensable (`sdkconfig.defaults`)

L'écran a besoin de la **PSRAM** pour son frame buffer. Sans elle, le firmware plante au démarrage
(`lcd_panel.rgb: no mem for frame buffer` puis Guru Meditation) : **écran noir**.

`idf.py set-target` (ou *ESP-IDF: Set Espressif Device Target* dans VS Code) régénère `sdkconfig`.
Les valeurs indispensables (PSRAM octale 80 MHz, flash 16 Mo, `partitions_lvgl.csv`, CPU 240 MHz,
démos LVGL) sont donc reprises dans `sdkconfig.defaults` et réappliquées automatiquement.
