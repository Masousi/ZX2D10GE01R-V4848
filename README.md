# ZX2D10GE01R
ZX2D10GE01R base on https://github.com/espressif/esp-idf/tree/release/v5.0

<!-- ⚫ AJOUTÉ (24/10/2026) : usage du projet, démos et pièges de configuration -->
## Matériel / environnement

- ESP32-S3 N16R8 : flash 16 Mo, PSRAM **octale** 8 Mo, écran RGB 480×480 (frame buffer en PSRAM, `fb_in_psram`) + encodeur MT8901.
<!-- ⚫ AJOUTÉ (24/10/2026) -->
- Encodeur : rotation sur GPIO 5/6 (MT8901), bouton-poussoir sur GPIO 3 (actif bas), anneau de **13 LED WS2812** sur GPIO 4.
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
| Widgets | `LV_USE_DEMO_WIDGETS` (+ `LV_DEMO_WIDGETS_SLIDESHOW`, sans effet en LVGL 8.3.3) et **défilement automatique des onglets** propre au projet (`APP_WIDGETS_AUTOPLAY_MS`, 5 s par défaut) |
| Keypad and encoder | `LV_USE_DEMO_KEYPAD_AND_ENCODER` |
| Benchmark | `LV_USE_DEMO_BENCHMARK` + `LV_DEMO_BENCHMARK_RGB565A8` (images adaptées au 16 bits) |
| Stress | `LV_USE_DEMO_STRESS` |
| Music | `LV_USE_DEMO_MUSIC` + `LV_DEMO_MUSIC_SQUARE` (480×480) + `LV_DEMO_MUSIC_AUTO_PLAY` |

Restent manuels : la base matérielle (PSRAM, flash, partitions, mémoire et logs LVGL), déjà fixée par `sdkconfig.defaults`,
et la désactivation éventuelle d'options contraires (ex. *Music → Enable Landscape* doit rester décoché).

| Démo | Interaction | Ce qu'on doit voir |
|---|---|---|
| **Widgets** | ✅ encodeur | onglets Profile / Analytics / Shop qui défilent seuls toutes les 5 s (en pause pendant l'utilisation de l'encodeur) |
| **Keypad and encoder** | ✅ **encodeur** (rotation + appui) | formulaire : la rotation déplace le focus, l'appui valide ou entre en édition |
| **Benchmark** | aucune | ~30 scènes animées enchaînées, puis un tableau de résultats (FPS moyen, temps CPU) ; mesuré sur cette carte : **28 FPS pondérés** |
| **Stress** | aucune | création/suppression d'objets et d'animations en boucle ; la carte ne doit ni planter ni redémarrer |
| **Music** | ✅ encodeur (boutons du lecteur) | lecteur de musique dont le scénario se déroule seul (*Auto play*) |

<!-- ⚫ MODIFIÉ (24/10/2026) : groupe LVGL par défaut + bouton actif bas -> l'encodeur pilote toutes les démos interactives -->
> **Encodeur uniquement, pas de tactile.** Le bouton rotatif (MT8901, GPIO 5/6) et son bouton-poussoir (GPIO 3)
> sont déclarés comme encodeur LVGL dans `main/screen.c`, et `main/main.c` crée un groupe LVGL par défaut :
> **tourner** déplace le focus, **appuyer** active l'élément ou passe en édition (la rotation change alors la
> valeur), **appuyer à nouveau** valide. *Benchmark* et *Stress* ne sont pas interactives.

<!-- ⚫ AJOUTÉ (24/10/2026) : anneau de LED (main/encoder_led.c) -->
> **Anneau de LED.** Les 13 LED de l'encodeur suivent la rotation (traînée **cyan** dans un sens, **orange**
> dans l'autre) et font un **flash blanc** à l'appui. Réglages dans *menuconfig → ZX2D10 Demo → Encoder LED ring* :
> nombre de LED, sens inversé, luminosité max (255 = 100 % par défaut).
>
> <!-- ⚫ AJOUTÉ (24/10/2026) : chenillard d'inactivité -->
> **Chenillard.** Après **5 s** sans toucher l'encodeur, l'anneau enchaîne en boucle 7 effets multicolores
> en fondu enchaîné (9 s chacun) : arc-en-ciel tournant, comète arc-en-ciel, comètes croisées de couleurs
> complémentaires, respiration, étincelles, théâtre, plasma. Toute rotation ou tout appui l'interrompt
> immédiatement. Délai réglable : *Encoder LED ring → Idle rainbow show after N seconds*.

<!-- ⚫ AJOUTÉ (24/10/2026) : récapitulatif des options propres au projet -->
## Options du projet (`menuconfig` → **ZX2D10 Demo**)

| Option | Défaut | Rôle |
|---|---|---|
| *Demo to run* | Music | démo lancée au démarrage (active automatiquement ses options LVGL) |
| *Widgets: change tab every N ms when idle* | 5000 | défilement automatique des onglets de Widgets (0 = désactivé) |
| *Encoder LED ring → Animate the encoder LEDs* | ✅ | animation de l'anneau (rotation, appui, chenillard) |
| *Encoder LED ring → Number of LEDs* | 13 | nombre de LED de l'anneau |
| *Encoder LED ring → Reverse direction* | ❌ | inverse le sens de la lumière |
| *Encoder LED ring → Max brightness* | 255 (100 %) | luminosité maximale des LED |
| *Encoder LED ring → Idle rainbow show after N seconds* | 5 | délai avant le chenillard multicolore |

## Tester une démo

### Option A : flasher un firmware fourni (sans compiler)

<!-- ⚫ MODIFIÉ (24/10/2026) : un firmware par démo -->
Un firmware par démo est fourni dans [`firmware/`](firmware/README.md) :
`ZX2D10GE01R_demo_widgets.bin`, `…_keypad_encoder.bin`, `…_benchmark.bin`, `…_stress.bin`, `…_music.bin`.

```sh
esptool.py --chip esp32s3 -p COM4 -b 460800 write_flash 0x0 firmware/ZX2D10GE01R_demo_widgets.bin
```

Remplacer le nom du fichier par la démo voulue et `COM4` par le port de la carte (puce USB-série CH343).

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
| L'encodeur ne fait rien | bouton déclaré actif haut (`button_attch(…, 1, …)`) : LVGL le croit appuyé et ignore la rotation | `button_attch(BTN_PIN, 0, 10)` dans `main/screen.c` (corrigé) |
| Écran noir dès qu'une démo s'anime (1re image correcte puis noir) | `full_refresh` désactivé alors que les buffers LVGL sont les frame buffers du panneau | `disp_drv.full_refresh = 1` dans `main/screen.c` (corrigé) |
| `Rendering this widget needs LV_COLOR_SCREEN_TRANSP 1` | widgets semi-transparents non dessinés (Music, Benchmark) | *LVGL → Color settings* → `LV_COLOR_SCREEN_TRANSP` (activé dans `sdkconfig.defaults`) |
| La lumière des LED tourne à l'envers | sens de l'anneau | *Encoder LED ring → Reverse direction* |
| La lumière saute ou ne fait pas un tour complet | nombre de LED incorrect | *Encoder LED ring → Number of LEDs* |
| La carte redémarre à l'appui sur le bouton | flash blanc des 13 LED à 100 % ≈ 0,8 A : alimentation USB insuffisante | port/chargeur USB plus puissant, ou *Encoder LED ring → Max brightness* plus bas |
| Le chenillard ne démarre jamais | encodeur considéré comme actif en permanence | vérifier le bouton (GPIO 3 doit valoir 1 au repos) |
| Music : interface tronquée ou décalée | *Landscape* ou *Round* coché en plus de *Square* | *LVGL → Demos* → décocher *Enable Landscape* / *Enable Round* |

<!-- ⚫ AJOUTÉ (24/10/2026) : pièges rencontrés avec l'extension VS Code ESP-IDF -->
## Compiler depuis VS Code (extension ESP-IDF)

- Version d'ESP-IDF : **v5.0.9** (*ESP-IDF: Select Current ESP-IDF Version*), cible **esp32s3**
  (*ESP-IDF: Set Espressif Device Target*). ⚠️ Changer la cible régénère `sdkconfig` : `sdkconfig.defaults`
  remet les valeurs indispensables, mais vérifier ensuite *Demo to run*.
- Port **COM4** (CH343), méthode de flash **UART**.
- Dans un espace de travail multi-dossiers : *ESP-IDF: Pick a Workspace Folder* → ce projet, avant Build/Flash.
- **Un seul build à la fois** : deux builds simultanés (extension + terminal) sur le même dossier `build/`
  provoquent des erreurs `ninja: failed recompaction: Permission denied` ou des `.obj` introuvables.
- Le moniteur série occupe COM4 : le fermer (`Ctrl+]`) avant un flash lancé ailleurs.

<!-- ⚫ AJOUTÉ (24/10/2026) : organisation du code -->
## Organisation du code

| Fichier | Rôle |
|---|---|
| `main/main.c` | tâche LVGL, choix de la démo, groupe encodeur, défilement auto de Widgets |
| `main/screen.c` | initialisation du panneau RGB 480×480 (`full_refresh`), encodeur LVGL (rotation + bouton) |
| `main/encoder_led.c` | animation de l'anneau de LED : suivi de la rotation, flash, chenillard d'inactivité |
| `main/Kconfig.projbuild` | menu **ZX2D10 Demo** (démo, défilement, LED) |
| `components/mt8901` | lecture de la rotation (compteur d'impulsions PCNT, pilote `driver/pulse_cnt.h`) |
| `components/button` | bouton-poussoir (anti-rebond) |
| `components/ws2812` | pilote des LED WS2812 via le périphérique RMT |
| `components/lvgl` | LVGL 8.3.3 et ses démos |
| `firmware/` | un firmware prêt à flasher par démo |
| `sdkconfig.defaults` | valeurs indispensables réappliquées si `sdkconfig` est régénéré |

## ⚠️ Configuration indispensable (`sdkconfig.defaults`)

L'écran a besoin de la **PSRAM** pour son frame buffer. Sans elle, le firmware plante au démarrage
(`lcd_panel.rgb: no mem for frame buffer` puis Guru Meditation) : **écran noir**.

`idf.py set-target` (ou *ESP-IDF: Set Espressif Device Target* dans VS Code) régénère `sdkconfig`.
Les valeurs indispensables (PSRAM octale 80 MHz, flash 16 Mo, `partitions_lvgl.csv`, CPU 240 MHz,
démos LVGL) sont donc reprises dans `sdkconfig.defaults` et réappliquées automatiquement.
