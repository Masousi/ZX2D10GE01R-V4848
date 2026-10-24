<!-- ⚫ AJOUTÉ (24/10/2026) : firmware prêt à flasher, sans avoir à compiler -->
# Firmware précompilé

| Fichier | Démo | Contenu | Adresse |
|---|---|---|---|
| `ZX2D10GE01R_demo_music.bin` | LVGL **Music** (mise en page carrée 480×480) | image complète : bootloader + table de partitions + application | `0x0` |

- Cible : ESP32-S3 N16R8 (flash 16 Mo, PSRAM octale 8 Mo), ESP-IDF v5.0.9.
- SHA-256 : `88002a7d563ee16a0a6ed51c5e9e56dc10f45b3597913751a755a69ade04b76c`

## Flasher

```sh
esptool.py --chip esp32s3 -p COM4 -b 460800 write_flash 0x0 ZX2D10GE01R_demo_music.bin
```

Ou avec l'outil web : https://espressif.github.io/esptool-js/ (adresse `0x0`).

## Régénérer après un changement de démo

1. `idf.py menuconfig` → **ZX2D10 Demo** → *Demo to run*
2. `idf.py build`
3. Dans `build/` :
   ```sh
   esptool.py --chip esp32s3 merge_bin -o ../firmware/ZX2D10GE01R_demo_<nom>.bin @flash_args
   ```
