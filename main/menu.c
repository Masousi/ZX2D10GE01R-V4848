// ⚫ AJOUTÉ (24/10/2026) : menu principal (menuconfig > ZX2D10 Demo > Demo to run > Main menu).
// - tourner l'encodeur : parcourir les démos ; appuyer : lancer la démo (redémarrage logiciel, ~1 s) ;
// - dans une démo : maintenir le bouton APP_MENU_BACK_HOLD_MS (3 s par défaut) -> retour au menu,
//   avec un anneau de progression affiché par-dessus la démo (lv_layer_top) ;
// - un plantage / reset matériel pendant une démo ramène au menu (la demande n'est valable qu'une fois).
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "lvgl.h"
#include "board.h"
#include "menu.h"

// Noms des démos : utilisés par les deux types de firmware (logs de main.c, liste du menu)
static const char * const s_names[APP_DEMO_COUNT] = {
    [APP_DEMO_WIDGETS]        = "Widgets",
    [APP_DEMO_KEYPAD_ENCODER] = "Keypad & Encoder",
    [APP_DEMO_BENCHMARK]      = "Benchmark",
    [APP_DEMO_STRESS]         = "Stress",
    [APP_DEMO_MUSIC]          = "Music",
    [APP_DEMO_LED]            = "Animations LED",
};

const char * app_menu_demo_name(app_demo_t demo)
{
    return (demo >= 0 && demo < APP_DEMO_COUNT) ? s_names[demo] : "Menu";
}

#if CONFIG_APP_DEMO_RUN_MENU

#define TAG "MENU"

#define BOOT_MAGIC     0x5A2D10DEu
#define HINT_DELAY_MS  700           // l'anneau de retour n'apparaît qu'après ce délai (appuis normaux invisibles)

// Mémoire RTC non initialisée : conservée par esp_restart(), aléatoire à la mise sous tension (d'où le magic)
RTC_NOINIT_ATTR static uint32_t s_boot_magic;
RTC_NOINIT_ATTR static int32_t s_boot_demo;

static app_demo_t s_current = APP_DEMO_NONE;   // démo en cours (NONE = menu affiché)

// Polices avec accents (générées par lv_font_conv, partagées avec demo_led.c)
LV_FONT_DECLARE(font_fr_20);
LV_FONT_DECLARE(font_fr_28);

// Démos réellement compilées (le choix "Main menu" les active toutes, mais on reste prudent)
static bool demo_available(app_demo_t d)
{
    switch (d) {
    case APP_DEMO_WIDGETS:        return LV_USE_DEMO_WIDGETS;
    case APP_DEMO_KEYPAD_ENCODER: return LV_USE_DEMO_KEYPAD_AND_ENCODER;
    case APP_DEMO_BENCHMARK:      return LV_USE_DEMO_BENCHMARK;
    case APP_DEMO_STRESS:         return LV_USE_DEMO_STRESS;
    case APP_DEMO_MUSIC:          return LV_USE_DEMO_MUSIC;
    case APP_DEMO_LED:            return true;
    default:                      return false;
    }
}

app_demo_t app_menu_requested(void)
{
    app_demo_t d = APP_DEMO_NONE;
    if (s_boot_magic == BOOT_MAGIC && esp_reset_reason() == ESP_RST_SW &&
        s_boot_demo >= 0 && s_boot_demo < APP_DEMO_COUNT && demo_available(s_boot_demo)) {
        d = (app_demo_t)s_boot_demo;
    }
    s_boot_magic = 0;   // consommée : le prochain redémarrage (retour, plantage) affiche le menu
    s_current = d;
    return d;
}

// Écran noir puis redémarrage (évite d'afficher une image figée pendant le reboot)
static void restart_clean(void)
{
    gpio_set_level(LCD_PIN_BK_LIGHT, LCD_BK_LIGHT_OFF_LEVEL);
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_restart();
}

void app_menu_launch(app_demo_t demo)
{
    ESP_LOGI(TAG, "Lancement : %s (redemarrage)", app_menu_demo_name(demo));
    s_boot_demo = demo;
    s_boot_magic = BOOT_MAGIC;
    restart_clean();
}

/* ---------- menu ---------- */

static app_demo_t s_ids[APP_DEMO_COUNT];   // index du roller -> démo
static uint32_t s_id_cnt;
static lv_obj_t * s_info;

static void launch_timer_cb(lv_timer_t * t)
{
    app_menu_launch((app_demo_t)(intptr_t)t->user_data);
}

// Appui sur l'encodeur : VALUE_CHANGED du roller (cf. demo_led.c)
static void menu_roller_cb(lv_event_t * e)
{
    uint16_t id = lv_roller_get_selected(lv_event_get_target(e));
    if (id >= s_id_cnt) return;
    app_demo_t d = s_ids[id];
    // "..." et non "…" (U+2026, absent des polices générées : plage 0x20-0x7F + 0xA0-0xFF)
    lv_label_set_text_fmt(s_info, "Lancement : %s...", app_menu_demo_name(d));
    // laisse LVGL afficher le message avant le redémarrage
    lv_timer_t * t = lv_timer_create(launch_timer_cb, 150, (void *)(intptr_t)d);
    lv_timer_set_repeat_count(t, 1);
}

void app_menu_show(void)
{
    s_current = APP_DEMO_NONE;
    ESP_LOGI(TAG, "Menu principal");

    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0E0F1A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Démos ZX2D10");
    lv_obj_set_style_text_font(title, &font_fr_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 52);

    static char opts[160];
    opts[0] = '\0';
    s_id_cnt = 0;
    uint16_t sel = 0;
    for (int d = 0; d < APP_DEMO_COUNT; d++) {
        if (!demo_available(d)) continue;
        if (s_id_cnt) strcat(opts, "\n");
        strcat(opts, s_names[d]);
        if (d == s_boot_demo) sel = s_id_cnt;   // re-sélectionne la dernière démo lancée
        s_ids[s_id_cnt++] = d;
    }

    lv_obj_t * roller = lv_roller_create(scr);
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(roller, 5);
    lv_obj_set_width(roller, 340);
    lv_obj_center(roller);
    lv_obj_set_style_text_font(roller, &font_fr_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0x8A8FA8), LV_PART_MAIN);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(roller, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(roller, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(roller, 0, LV_PART_MAIN | LV_STATE_EDITED);
    lv_obj_set_style_text_font(roller, &font_fr_28, LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_white(), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x2A6DF4), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_radius(roller, 14, LV_PART_SELECTED);
    lv_roller_set_selected(roller, sel, LV_ANIM_OFF);
    lv_obj_add_event_cb(roller, menu_roller_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_info = lv_label_create(scr);
    lv_label_set_text_fmt(s_info, "Appui long %d s : menu", CONFIG_APP_MENU_BACK_HOLD_MS / 1000);
    lv_obj_set_style_text_font(s_info, &font_fr_20, 0);
    lv_obj_set_style_text_color(s_info, lv_color_hex(0x7FD6FF), 0);
    lv_obj_align(s_info, LV_ALIGN_BOTTOM_MID, 0, -60);

    // seul objet du groupe par défaut : mode édition direct -> la rotation fait défiler la liste
    lv_group_t * g = lv_obj_get_group(roller);
    if (g) {
        lv_group_focus_obj(roller);
        lv_group_set_editing(g, true);
    }
}

/* ---------- retour au menu (appui long) ---------- */

static lv_obj_t * s_hint;       // anneau de progression sur lv_layer_top (au-dessus de toute démo)
static lv_obj_t * s_hint_arc;

static void hint_show(uint32_t held_ms)
{
    if (!s_hint) {
        s_hint = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(s_hint);
        lv_obj_set_size(s_hint, 200, 200);
        lv_obj_center(s_hint);
        lv_obj_set_style_radius(s_hint, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_hint, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(s_hint, LV_OPA_80, 0);
        lv_obj_clear_flag(s_hint, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        s_hint_arc = lv_arc_create(s_hint);
        lv_obj_set_size(s_hint_arc, 180, 180);
        lv_obj_center(s_hint_arc);
        lv_arc_set_rotation(s_hint_arc, 270);
        lv_arc_set_bg_angles(s_hint_arc, 0, 360);
        lv_arc_set_range(s_hint_arc, 0, 100);
        lv_obj_remove_style(s_hint_arc, NULL, LV_PART_KNOB);
        lv_obj_clear_flag(s_hint_arc, LV_OBJ_FLAG_CLICKABLE);
        lv_group_remove_obj(s_hint_arc);   // ne doit pas voler le focus de l'encodeur
        lv_obj_set_style_arc_color(s_hint_arc, lv_color_hex(0x2A6DF4), LV_PART_INDICATOR);

        lv_obj_t * lbl = lv_label_create(s_hint);
        lv_label_set_text(lbl, "Menu");
        lv_obj_set_style_text_font(lbl, &font_fr_28, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_center(lbl);
    }
    lv_obj_clear_flag(s_hint, LV_OBJ_FLAG_HIDDEN);
    uint32_t span = CONFIG_APP_MENU_BACK_HOLD_MS - HINT_DELAY_MS;
    lv_arc_set_value(s_hint_arc, (int16_t)((held_ms - HINT_DELAY_MS) * 100 / span));
}

static void hint_hide(void)
{
    if (s_hint) lv_obj_add_flag(s_hint, LV_OBJ_FLAG_HIDDEN);
}

void app_menu_poll(void)
{
    static bool held;
    static uint32_t t0;

    if (s_current == APP_DEMO_NONE) return;            // dans le menu : l'appui sert à lancer une démo

    bool pressed = (gpio_get_level(BTN_PIN) == 0);     // bouton actif à l'état bas
    if (!pressed) {
        if (held) hint_hide();
        held = false;
        return;
    }
    if (!held) {
        held = true;
        t0 = lv_tick_get();
        return;
    }
    uint32_t el = lv_tick_elaps(t0);
    if (el >= CONFIG_APP_MENU_BACK_HOLD_MS) {
        ESP_LOGI(TAG, "Appui long : retour au menu");
        restart_clean();
    } else if (el >= HINT_DELAY_MS) {
        hint_show(el);
    }
}

#endif
