// ⚫ AJOUTÉ (24/10/2026) : liste des démos (commune aux deux types de firmware) et menu principal (main/menu.c).
// Choix : menuconfig > ZX2D10 Demo > Demo to run
// - Main menu   : choix de la démo à l'écran ; la démo est mémorisée en RTC (survit à esp_restart) puis
//                 lancée après un redémarrage logiciel : état LVGL propre (LVGL 8.3 ne sait pas fermer une démo) ;
// - autre choix  : une seule démo dans le firmware (APP_DEMO_SINGLE ci-dessous).
#pragma once

#include "sdkconfig.h"

typedef enum {
    APP_DEMO_NONE = -1,            // pas de démo -> menu (Main menu) / avertissement (démo seule)
    APP_DEMO_WIDGETS = 0,
    APP_DEMO_KEYPAD_ENCODER,
    APP_DEMO_BENCHMARK,
    APP_DEMO_STRESS,
    APP_DEMO_MUSIC,
    APP_DEMO_LED,
    APP_DEMO_COUNT
} app_demo_t;

// Firmware à démo seule : démo choisie dans menuconfig ("Demo to run")
#if defined(CONFIG_APP_DEMO_RUN_WIDGETS)
#define APP_DEMO_SINGLE APP_DEMO_WIDGETS
#elif defined(CONFIG_APP_DEMO_RUN_KEYPAD_ENCODER)
#define APP_DEMO_SINGLE APP_DEMO_KEYPAD_ENCODER
#elif defined(CONFIG_APP_DEMO_RUN_BENCHMARK)
#define APP_DEMO_SINGLE APP_DEMO_BENCHMARK
#elif defined(CONFIG_APP_DEMO_RUN_STRESS)
#define APP_DEMO_SINGLE APP_DEMO_STRESS
#elif defined(CONFIG_APP_DEMO_RUN_MUSIC)
#define APP_DEMO_SINGLE APP_DEMO_MUSIC
#elif defined(CONFIG_APP_DEMO_RUN_LED)
#define APP_DEMO_SINGLE APP_DEMO_LED
#else
#define APP_DEMO_SINGLE APP_DEMO_NONE
#endif

// Nom affiché d'une démo (les deux types de firmware).
const char * app_menu_demo_name(app_demo_t demo);

#if defined(CONFIG_APP_DEMO_RUN_MENU)
// Démo demandée avant le redémarrage (une seule fois : un plantage ou un reset ramène au menu).
app_demo_t app_menu_requested(void);

// Affiche le menu principal.
void app_menu_show(void);

// Mémorise la démo et redémarre pour la lancer.
void app_menu_launch(app_demo_t demo);

// À appeler dans la boucle LVGL : appui long (APP_MENU_BACK_HOLD_MS) -> retour au menu.
void app_menu_poll(void);
#endif
