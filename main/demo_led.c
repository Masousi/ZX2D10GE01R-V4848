// ⚫ AJOUTÉ (24/10/2026) : démo "LED animations" : choix de l'animation de l'anneau de LED depuis l'écran.
// - tourner l'encodeur : parcourir la liste (roller circulaire, contenu centré sur l'écran 480x480) ;
// - appuyer : valider -> l'animation démarre immédiatement sur l'anneau.
// Pendant qu'on tourne, l'anneau affiche la comète interactive ; l'animation choisie revient
// après APP_ENCODER_LED_IDLE_S secondes sans action (menuconfig > ZX2D10 Demo > Encoder LED ring).
#include <string.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "esp_log.h"
#include "encoder_led.h"

// compilée seulement si elle peut être lancée (Demo to run = Main menu ou LED animations)
#if defined(CONFIG_APP_DEMO_RUN_MENU) || defined(CONFIG_APP_DEMO_RUN_LED)

#define TAG "DEMO_LED"

// Polices Montserrat avec accents (é, è, ê, î, â, É…) : les polices intégrées de LVGL 8.3 sont ASCII seulement.
// Générées par lv_font_conv (voir l'en-tête de font_fr_20.c / font_fr_28.c).
LV_FONT_DECLARE(font_fr_20);
LV_FONT_DECLARE(font_fr_28);

// Ordre = index des animations dans encoder_led.c (0..7, Random = ENC_LED_FX_RANDOM)
static const char * const s_names[] = {
    "Arc-en-ciel tournant",
    "Comète arc-en-ciel",
    "Comètes croisées",
    "Respiration",
    "Étincelles",
    "Théâtre",
    "Plasma",
    "Random",
};
#define NAME_COUNT (sizeof(s_names) / sizeof(s_names[0]))

static lv_obj_t * s_status;

static void show_status(int mode)
{
    if (mode >= 0 && mode < (int)NAME_COUNT) {
        lv_label_set_text_fmt(s_status, LV_SYMBOL_OK " %s", s_names[mode]);
    } else {
        lv_label_set_text(s_status, "Enchaînement automatique");
    }
}

// Appui sur l'encodeur : le roller envoie VALUE_CHANGED (lv_roller.c, release_handler)
static void roller_event_cb(lv_event_t * e)
{
    lv_obj_t * roller = lv_event_get_target(e);
    uint16_t id = lv_roller_get_selected(roller);
    if (id >= ENC_LED_FX_COUNT) return;
    int mode = id;

    encoder_led_set_mode(mode);
    show_status(mode);
    ESP_LOGI(TAG, "Animation : %s", s_names[id]);

    // Après validation, LVGL sort le roller du mode édition : on y revient tout de suite
    // pour que la rotation continue de parcourir la liste (sinon il faudrait un appui de plus).
    lv_group_set_editing(lv_obj_get_group(roller), true);
}

void demo_led(void)
{
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0E0F1A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Titre (contenu resserré au centre, lisible même si les coins de l'écran sont masqués)
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Animations LED");
    lv_obj_set_style_text_font(title, &font_fr_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 52);

    // Liste circulaire des animations
    static char opts[256];
    opts[0] = '\0';
    for (size_t i = 0; i < NAME_COUNT; i++) {
        if (i) strcat(opts, "\n");
        strcat(opts, s_names[i]);
    }
    lv_obj_t * roller = lv_roller_create(scr);
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(roller, 5);
    lv_obj_set_width(roller, 360);
    lv_obj_center(roller);
    // lignes normales
    lv_obj_set_style_text_font(roller, &font_fr_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0x8A8FA8), LV_PART_MAIN);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(roller, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(roller, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(roller, 0, LV_PART_MAIN | LV_STATE_EDITED);
    // ligne sélectionnée
    lv_obj_set_style_text_font(roller, &font_fr_28, LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_white(), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x2A6DF4), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_radius(roller, 14, LV_PART_SELECTED);

    int mode = encoder_led_get_mode();
    if (mode >= 0 && mode < (int)NAME_COUNT) lv_roller_set_selected(roller, mode, LV_ANIM_OFF);
    lv_obj_add_event_cb(roller, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Animation active
    s_status = lv_label_create(scr);
    lv_obj_set_style_text_font(s_status, &font_fr_20, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x7FD6FF), 0);
    lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, -60);
    show_status(mode);

    // Le roller est ajouté au groupe par défaut (main.c, encoder_group_init) : seul objet du groupe,
    // on le met directement en mode édition -> la rotation fait défiler la liste dès le départ.
    lv_group_t * g = lv_obj_get_group(roller);
    if (g) {
        lv_group_focus_obj(roller);
        lv_group_set_editing(g, true);
    } else {
        ESP_LOGW(TAG, "Pas de groupe LVGL : l'encodeur ne pilotera pas la liste");
    }
}

#endif
