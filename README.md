# ZX2D10GE01R
ZX2D10GE01R base on https://github.com/espressif/esp-idf/tree/release/v5.0

<!-- ⚫ AJOUTÉ (24/10/2026) : usage du projet, démos et pièges de configuration -->
## Matériel / environnement

- ESP32-S3 N16R8 : flash 16 Mo, PSRAM **octale** 8 Mo, écran RGB 480×480 (frame buffer en PSRAM, `fb_in_psram`) + encodeur MT8901.
- ESP-IDF **v5.0.9**.

## Démos LVGL disponibles

<!-- ⚫ MODIFIÉ (24/10/2026) : la procédure Music est généralisée à toutes les démos (LVGL 8.3) -->
Une seule démo tourne à la fois : celle choisie dans `idf.py menuconfig` → **ZX2D10 Demo** → *Demo to run*
(Music par défaut).

<!-- ⚫ AJOUTÉ (24/10/2026) : options LVGL activées automatiquement (main/Kconfig.projbuild, `select`) -->
**Les autres paramètres sont automatiques.** Choisir une démo dans *Demo to run* active automatiquement
la démo LVGL correspondante et ses options adaptées à cette carte. Il n'est plus nécessaire de passer par
*Component config → LVGL → Demos* : les options ci-dessous y apparaissent cochées et verrouillées.

| *Demo to run* | Activé automatiquement |
|---|---|
| Widgets | `LV_USE_DEMO_WIDGETS` + `LV_DEMO_WIDGETS_SLIDESHOW` (défilement automatique des onglets) |
| Keypad and encoder | `LV_USE_DEMO_KEYPAD_AND_ENCODER` |
| Benchmark | `LV_USE_DEMO_BENCHMARK` + `LV_DEMO_BENCHMARK_RGB565A8` (images adaptées au 16 bits) |
| Stress | `LV_USE_DEMO_STRESS` |
| Music | `LV_USE_DEMO_MUSIC` + `LV_DEMO_MUSIC_SQUARE` (480×480) + `LV_DEMO_MUSIC_AUTO_PLAY` |

Restent manuels : la base matérielle (PSRAM, flash, partitions, mémoire et logs LVGL), déjà fixée par `sdkconfig.defaults`,
et la désactivation éventuelle d'options contraires (ex. *Music → Enable Landscape* doit rester décoché).

| Démo | Interaction | Ce qu'on doit voir |
|---|---|---|
| **Widgets** | aucune (voir encadré) | onglets Profile / Analytics / Shop qui défilent automatiquement |
| **Keypad and encoder** | ✅ **encodeur** (rotation + appui) | formulaire : la rotation déplace le focus, l'appui valide ou entre en édition |
| **Benchmark** | aucune | ~30 scènes animées enchaînées, puis un tableau de résultats (FPS moyen, temps CPU) |
| **Stress** | aucune | création/suppression d'objets et d'animations en boucle ; la carte ne doit ni planter ni redémarrer |
| **Music** | aucune | lecteur de musique dont le scénario se déroule seul (*Auto play*) |

> **Encodeur uniquement, pas de tactile.** Le bouton rotatif (MT8901, GPIO 5/6) et son bouton-poussoir (GPIO 3)
> sont déclarés comme encodeur LVGL dans `main/screen.c`. Le projet ne définit aucun groupe LVGL par défaut :
> seule la démo *Keypad and encoder*, qui crée son propre groupe, réagit à l'encodeur. Les autres démos ne font
> qu'afficher, d'où les options *slide show* / *Auto play* activées automatiquement pour voir quelque chose bouger.

## Tester une démo

### Option A : flasher un firmware fourni (sans compiler)

Seule la démo Music est fournie pour l'instant (voir [`firmware/`](firmware/README.md)) :

```sh
esptool.py --chip esp32s3 -p COM4 -b 460800 write_flash 0x0 firmware/ZX2D10GE01R_demo_music.bin
```

Remplacer `COM4` par le port de la carte (puce USB-série CH343).

### Option B : compiler la démo de son choix

1. **Ouvrir la configuration** : `idf.py menuconfig`
   (dans VS Code : `Ctrl+Shift+P` → *ESP-IDF: SDK Configuration Editor (menuconfig)*).

2. **Contrôler la base matérielle**, commune à toutes les démos (déjà réglée) :

   | Menu | Option | Valeur |
   |---|---|---|
   | Component config → ESP PSRAM | Support for external, SPI-connected RAM | ✅, mode **Octal**, **80 MHz** |
   | Serial flasher config | Flash size | **16 MB** |
   | Partition Table | Custom partition table CSV | `partitions_lvgl.csv` (app `factory` de **8 Mo** : contient toutes les démos et leurs images) |
   | Component config → LVGL → Memory settings | If true use custom malloc/free… (`LV_MEM_CUSTOM`) | ✅ : LVGL alloue dans le heap ESP-IDF (PSRAM incluse) au lieu d'un tas fixe de 64 Ko |
   | Component config → LVGL → Log settings | Enable the log module | ✅, niveau **Warning**, *Print the log with printf* |

3. **Choisir la démo** : **ZX2D10 Demo** → *Demo to run*. Les options LVGL nécessaires s'activent automatiquement
   (voir le tableau plus haut). Enregistrer (`S`) puis quitter (`Q`).

4. **Compiler, flasher et ouvrir le moniteur série** :

   ```sh
   idf.py -p COM4 build flash monitor
   ```

   Dans VS Code, sans terminal : `Ctrl+Shift+P` → *Tasks: Run Task* → **ESP: Build + Flash + Monitor**
   (tâches dans `.vscode/tasks.json`, fichier local non versionné), ou les boutons Build / Flash / Monitor
   de l'extension ESP-IDF. Quitter le moniteur : `Ctrl+]`.

5. **Vérifier le journal série** au démarrage :

   ```
   esp_psram: Found 8MB PSRAM device
   esp_psram: Adding pool of 7360K of PSRAM memory to heap allocator
   cpu_start: cpu freq: 240000000 Hz
   MAIN: Demo: <widgets | keypad & encoder | benchmark | stress | music>
   ```

   La ligne `MAIN: Demo: …` confirme la démo réellement lancée.

6. *(Facultatif)* **Générer le `.bin` de la démo** pour la flasher sans compiler :

   ```sh
   cd build
   esptool.py --chip esp32s3 merge_bin -o ../firmware/ZX2D10GE01R_demo_<nom>.bin @flash_args
   ```

### Dépannage

| Symptôme | Cause probable | Solution |
|---|---|---|
| Écran noir + `no mem for frame buffer` + redémarrages | PSRAM désactivée (`sdkconfig` régénéré) | supprimer `sdkconfig`, puis `idf.py build` (reprend `sdkconfig.defaults`) |
| `SPI Flash Size : 2MB` dans le journal | flash mal configurée | *Serial flasher config → Flash size* = 16 MB |
| Erreur de build « app partition is too small » | table de partitions par défaut | *Partition Table* → `partitions_lvgl.csv` |
| Écran noir, aucune erreur ni redémarrage, journal arrêté après `MAIN: Demo: …` | tas LVGL fixe saturé (`LV_MEM_CUSTOM` désactivé) : LVGL se bloque en silence | activer `LV_MEM_CUSTOM` (*LVGL → Memory settings*) ; avec les logs LVGL activés, l'erreur s'affiche en `[Error]` |
| `Insufficient memory for lv_demo_widgets` à la compilation | `LV_MEM_CUSTOM` désactivé et mémoire LVGL < 38 Ko | activer `LV_MEM_CUSTOM` |
| `Aucune demo selectionnee/activee` dans le journal | `sdkconfig` antérieur à l'activation automatique | re-choisir la démo dans *ZX2D10 Demo*, enregistrer, rebuild |
| Démo différente de celle attendue | `sdkconfig` non enregistré avant le build | menuconfig → `S` pour enregistrer, puis rebuild |
| L'encodeur ne fait rien | démo autre que *Keypad and encoder* | comportement normal (aucun groupe LVGL par défaut) |
| Music : interface tronquée ou décalée | *Landscape* ou *Round* coché en plus de *Square* | *LVGL → Demos* → décocher *Enable Landscape* / *Enable Round* |

## ⚠️ Configuration indispensable (`sdkconfig.defaults`)

L'écran a besoin de la **PSRAM** pour son frame buffer. Sans elle, le firmware plante au démarrage
(`lcd_panel.rgb: no mem for frame buffer` puis Guru Meditation) : **écran noir**.

`idf.py set-target` (ou *ESP-IDF: Set Espressif Device Target* dans VS Code) régénère `sdkconfig`.
Les valeurs indispensables (PSRAM octale 80 MHz, flash 16 Mo, `partitions_lvgl.csv`, CPU 240 MHz,
démos LVGL) sont donc reprises dans `sdkconfig.defaults` et réappliquées automatiquement.
