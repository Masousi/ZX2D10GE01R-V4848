#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "rom/gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"
#include "board.h"
#include "esp_timer.h"
// ⚫ AJOUTÉ (24/10/2026) : prototypes des démos (chacun protégé par son LV_USE_DEMO_xxx)
#include "lvgl/demos/lv_demos.h"
// ⚫ AJOUTÉ (24/10/2026) : liste des démos + menu principal (main/menu.c)
#include "menu.h"

#define TAG "MAIN"

static void increase_lvgl_tick(void* arg) {
    lv_tick_inc(portTICK_PERIOD_MS);
}

extern void screen_init(void);
// ⚫ AJOUTÉ (24/10/2026) : animation de l'anneau de LED de l'encodeur (main/encoder_led.c)
extern void encoder_led_start(void);
// ⚫ AJOUTÉ (24/10/2026) : démo "LED animations" (main/demo_led.c)
extern void demo_led(void);

// ⚫ AJOUTÉ (24/10/2026) : groupe LVGL par défaut relié à l'encodeur.
// Sans groupe, LVGL ignore l'encodeur : seule la démo "Keypad and encoder" (qui crée son propre groupe)
// réagissait. Les widgets créés ensuite (boutons, curseurs, onglets…) y sont ajoutés automatiquement :
// tourner = déplacer le focus, appuyer = activer / entrer en édition (rotation = changer la valeur),
// appuyer à nouveau = valider.
static void encoder_group_init(void) {
    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    for (lv_indev_t * indev = lv_indev_get_next(NULL); indev; indev = lv_indev_get_next(indev)) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
            lv_indev_set_group(indev, g);
        }
    }
}

// #if defined(CONFIG_APP_DEMO_RUN_WIDGETS) && LV_USE_DEMO_WIDGETS && CONFIG_APP_WIDGETS_AUTOPLAY_MS > 0
// ⚫ MODIFIÉ (24/10/2026) : aussi quand Widgets est lancé depuis le menu principal
#if (defined(CONFIG_APP_DEMO_RUN_WIDGETS) || defined(CONFIG_APP_DEMO_RUN_MENU)) && LV_USE_DEMO_WIDGETS && CONFIG_APP_WIDGETS_AUTOPLAY_MS > 0
// ⚫ AJOUTÉ (24/10/2026) : défilement automatique des onglets Profile / Analytics / Shop.
// LV_DEMO_WIDGETS_SLIDESHOW n'est pas implémenté dans LVGL 8.3.3 : sans cela la démo reste figée.
// Le défilement se met en pause tant que l'encodeur est utilisé (inactivité < 2 périodes).
static void widgets_autoplay_cb(lv_timer_t * t) {
    lv_obj_t * tv = (lv_obj_t *)t->user_data;
    if (lv_disp_get_inactive_time(NULL) < 2 * CONFIG_APP_WIDGETS_AUTOPLAY_MS) return;
    uint32_t cnt = lv_obj_get_child_cnt(lv_tabview_get_content(tv));
    if (cnt == 0) return;
    lv_tabview_set_act(tv, (lv_tabview_get_tab_act(tv) + 1) % cnt, LV_ANIM_ON);
}

static void widgets_autoplay_start(void) {
    // la démo crée son lv_tabview directement sur l'écran actif : on le retrouve parmi les enfants
    lv_obj_t * scr = lv_scr_act();
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(scr); i++) {
        lv_obj_t * child = lv_obj_get_child(scr, i);
        if (lv_obj_check_type(child, &lv_tabview_class)) {
            lv_timer_create(widgets_autoplay_cb, CONFIG_APP_WIDGETS_AUTOPLAY_MS, child);
            ESP_LOGI(TAG, "Widgets: defilement auto des onglets toutes les %d ms", CONFIG_APP_WIDGETS_AUTOPLAY_MS);
            return;
        }
    }
    ESP_LOGW(TAG, "Widgets: tabview introuvable, pas de defilement auto");
}
#endif

// ⚫ AJOUTÉ (24/10/2026) : lancement d'une démo, commun aux deux types de firmware :
// - démo seule : appelée au démarrage avec APP_DEMO_SINGLE (démo choisie dans menuconfig, cf. menu.h) ;
// - Main menu   : appelé après le redémarrage avec la démo choisie à l'écran.
// Chaque cas n'est compilé que si la démo l'est (LV_USE_DEMO_xxx) : rien n'est lié inutilement.
// Ajouter une démo : app_demo_t + nom (menu.h / menu.c), un "case" ici, une entrée dans Kconfig.projbuild.
static void app_demo_run(app_demo_t demo) {
    ESP_LOGI(TAG, "Demo: %s", app_menu_demo_name(demo));
    switch (demo) {
#if LV_USE_DEMO_WIDGETS
    case APP_DEMO_WIDGETS:
        lv_demo_widgets();
#if CONFIG_APP_WIDGETS_AUTOPLAY_MS > 0
        widgets_autoplay_start();
#endif
        break;
#endif
#if LV_USE_DEMO_KEYPAD_AND_ENCODER
    case APP_DEMO_KEYPAD_ENCODER:
        lv_demo_keypad_encoder();
        break;
#endif
#if LV_USE_DEMO_BENCHMARK
    case APP_DEMO_BENCHMARK:
        lv_demo_benchmark();
        break;
#endif
#if LV_USE_DEMO_STRESS
    case APP_DEMO_STRESS:
        lv_demo_stress();
        break;
#endif
#if LV_USE_DEMO_MUSIC
    case APP_DEMO_MUSIC:
        lv_demo_music();
        break;
#endif
#if defined(CONFIG_APP_DEMO_RUN_MENU) || defined(CONFIG_APP_DEMO_RUN_LED)
    case APP_DEMO_LED:
        demo_led();
        break;
#endif
    default:
#if defined(CONFIG_APP_DEMO_RUN_MENU)
        app_menu_show();
#else
        ESP_LOGW(TAG, "Aucune demo selectionnee/activee (menuconfig > ZX2D10 Demo)");
#endif
        break;
    }
}

void lvgl_task(void* arg) {
    // ⚫ AJOUTÉ (24/10/2026) : screen_init() alloue le frame buffer RGB 480x480 en PSRAM.
    // Si CONFIG_SPIRAM est désactivé (sdkconfig régénéré), l'allocation échoue -> panic -> écran noir.
    // Les valeurs requises sont figées dans sdkconfig.defaults.
    screen_init();

    // Tick interface for LVGL
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = increase_lvgl_tick,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, portTICK_PERIOD_MS * 1000);

    encoder_group_init();
    encoder_led_start();

    // ⚫ MODIFIÉ (24/10/2026) : démo choisie dans menuconfig ("ZX2D10 Demo" -> "Demo to run"),
    // au lieu d'un appel fixe à lv_demo_benchmark() (erreur de link si le benchmark est désactivé).
    // extern void lv_demo_benchmark(void);
    // lv_demo_benchmark();
    // ⚫ MODIFIÉ (24/10/2026) : choix fait dans menuconfig ("ZX2D10 Demo" -> "Demo to run") : menu principal ou démo seule
#if defined(CONFIG_APP_DEMO_RUN_MENU)
    // menu principal, ou démo demandée juste avant le redémarrage
    app_demo_t demo = app_menu_requested();
    if (demo == APP_DEMO_NONE) {
        app_menu_show();
    } else {
        app_demo_run(demo);
    }
#else
    // une seule démo, choisie dans "Demo to run" (APP_DEMO_SINGLE, cf. menu.h)
    app_demo_run(APP_DEMO_SINGLE);
#endif

    // ⚫ 24/10/2026 : ancien aiguillage #if/#elif, conservé pour référence et désactivé :
    // remplacé par app_demo_run() (même liste de démos pour le menu principal et les démos seules).
#if 0
#if defined(CONFIG_APP_DEMO_RUN_WIDGETS) && LV_USE_DEMO_WIDGETS
    ESP_LOGI(TAG, "Demo: widgets");
    lv_demo_widgets();
#if CONFIG_APP_WIDGETS_AUTOPLAY_MS > 0
    widgets_autoplay_start();
#endif
#elif defined(CONFIG_APP_DEMO_RUN_KEYPAD_ENCODER) && LV_USE_DEMO_KEYPAD_AND_ENCODER
    ESP_LOGI(TAG, "Demo: keypad & encoder");
    lv_demo_keypad_encoder();
#elif defined(CONFIG_APP_DEMO_RUN_BENCHMARK) && LV_USE_DEMO_BENCHMARK
    ESP_LOGI(TAG, "Demo: benchmark");
    lv_demo_benchmark();
#elif defined(CONFIG_APP_DEMO_RUN_STRESS) && LV_USE_DEMO_STRESS
    ESP_LOGI(TAG, "Demo: stress");
    lv_demo_stress();
#elif defined(CONFIG_APP_DEMO_RUN_MUSIC) && LV_USE_DEMO_MUSIC
    ESP_LOGI(TAG, "Demo: music");
    lv_demo_music();
#else
    ESP_LOGW(TAG, "Aucune demo selectionnee/activee (menuconfig > ZX2D10 Demo)");
#endif
#endif // #if 0

    for (;;) {
#if defined(CONFIG_APP_DEMO_RUN_MENU)
        app_menu_poll();   // ⚫ AJOUTÉ (24/10/2026) : appui long -> retour au menu principal
#endif
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ⚫ AJOUTÉ (24/10/2026) : app_main rend la main tout de suite ; LVGL tourne dans sa propre tâche sur le cœur 1
void app_main(void) {
    xTaskCreatePinnedToCore(lvgl_task, NULL, 8 * 1024, NULL, 5, NULL, 1);
}
