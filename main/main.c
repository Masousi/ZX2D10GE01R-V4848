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

#define TAG "MAIN"

static void increase_lvgl_tick(void* arg) {
    lv_tick_inc(portTICK_PERIOD_MS);
}

extern void screen_init(void);

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

    // ⚫ MODIFIÉ (24/10/2026) : démo choisie dans menuconfig ("ZX2D10 Demo" -> "Demo to run"),
    // au lieu d'un appel fixe à lv_demo_benchmark() (erreur de link si le benchmark est désactivé).
    // extern void lv_demo_benchmark(void);
    // lv_demo_benchmark();
#if defined(CONFIG_APP_DEMO_RUN_WIDGETS) && LV_USE_DEMO_WIDGETS
    ESP_LOGI(TAG, "Demo: widgets");
    lv_demo_widgets();
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

    for (;;) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ⚫ AJOUTÉ (24/10/2026) : app_main rend la main tout de suite ; LVGL tourne dans sa propre tâche sur le cœur 1
void app_main(void) {
    xTaskCreatePinnedToCore(lvgl_task, NULL, 8 * 1024, NULL, 5, NULL, 1);
}
