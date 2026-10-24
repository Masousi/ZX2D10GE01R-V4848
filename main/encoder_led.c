// ⚫ AJOUTÉ (24/10/2026) : animation de l'anneau de LED (WS2812, GPIO LED_PIN) de l'encodeur.
// - rotation : une "comète" suit la position du bouton (tête lumineuse + traînée qui s'estompe),
//   couleur cyan dans un sens, orange dans l'autre ;
// - appui : flash blanc de tout l'anneau ;
// - au repos : extinction progressive.
// ⚫ AJOUTÉ (24/10/2026) : - inactivité (aucune rotation ni appui depuis APP_ENCODER_LED_IDLE_S) :
//   chenillard multicolore qui enchaîne 7 effets avec fondus enchaînés (arc-en-ciel tournant, comète
//   arc-en-ciel, comètes croisées, respiration, étincelles, théâtre, plasma). La moindre action sur
//   l'encodeur rend immédiatement la main à l'animation interactive.
// Réglages : menuconfig > ZX2D10 Demo > Encoder LED ring (nombre de LED, sens, luminosité : 255 = 100 % par défaut).
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "sdkconfig.h"
#include "board.h"
#include "mt8901.h"
#include "ws2812.h"

#if CONFIG_APP_ENCODER_LED

#define TAG "ENC_LED"

#define LED_COUNT     CONFIG_APP_ENCODER_LED_COUNT
#define LED_MAX       CONFIG_APP_ENCODER_LED_BRIGHTNESS   // luminosité max (0-255) : les WS2812 éblouissent vite
#define FRAME_MS      20                                  // 50 images/s
#define FADE_NUM      82                                  // traînée : intensité *= 82/100 à chaque image

// ⚫ 24/10/2026 : chenillard d'inactivité
#define IDLE_FRAMES        (CONFIG_APP_ENCODER_LED_IDLE_S * 1000 / FRAME_MS)  // délai avant le chenillard
#define SHOW_EFFECT_FRAMES (9000 / FRAME_MS)    // durée de chaque effet : 9 s
#define SHOW_XFADE_FRAMES  (1200 / FRAME_MS)    // fondu enchaîné entre deux effets : 1,2 s
#define SHOW_FADEIN_FRAMES (1500 / FRAME_MS)    // apparition / disparition du chenillard : 1,5 s
#define SHOW_EFFECTS       7

typedef struct { float r, g, b; } rgbf_t;      // couleur 0..1 (avant luminosité / gamma)

static uint8_t s_level[LED_COUNT];   // intensité courante de chaque LED (0..255)
static float s_spark[LED_COUNT];     // ⚫ 24/10/2026 : intensité des étincelles (effet 4)
static float s_spark_hue[LED_COUNT];

// ⚫ 24/10/2026 : teinte (0..1, cyclique) -> RGB saturé
static rgbf_t hsv(float h, float s, float v)
{
    h = h - floorf(h);
    float i = floorf(h * 6.0f), f = h * 6.0f - i;
    float p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    switch ((int)i % 6) {
        case 0:  return (rgbf_t){v, t, p};
        case 1:  return (rgbf_t){q, v, p};
        case 2:  return (rgbf_t){p, v, t};
        case 3:  return (rgbf_t){p, q, v};
        case 4:  return (rgbf_t){t, p, v};
        default: return (rgbf_t){v, p, q};
    }
}

static float ring_dist(float a, float b)   // distance circulaire entre deux positions (en LED)
{
    float d = fabsf(a - b);
    return fminf(d, LED_COUNT - d);
}

// ⚫ 24/10/2026 : calcule l'effet n°`fx` pour la LED `i` à l'instant `t` (en secondes)
static rgbf_t show_effect(int fx, int i, float t)
{
    const float n = LED_COUNT;
    switch (fx) {
    case 0: {   // arc-en-ciel complet qui tourne sur l'anneau
        return hsv(i / n + t * 0.25f, 1, 1);
    }
    case 1: {   // comète arc-en-ciel : tête brillante, longue traînée qui change de couleur
        float head = fmodf(t * 9.0f, n);
        float d = fmodf(head - i + n, n);                  // distance derrière la tête
        float v = expf(-d * 0.45f);
        return hsv(t * 0.15f - d * 0.04f, 1, v);
    }
    case 2: {   // deux comètes de couleurs complémentaires qui tournent en sens opposés et se croisent
        float a = fmodf(t * 6.0f, n), b = n - 1 - fmodf(t * 6.0f, n);
        float va = expf(-ring_dist(i, a) * 0.9f), vb = expf(-ring_dist(i, b) * 0.9f);
        rgbf_t ca = hsv(t * 0.1f, 1, va), cb = hsv(t * 0.1f + 0.5f, 1, vb);
        return (rgbf_t){fminf(ca.r + cb.r, 1), fminf(ca.g + cb.g, 1), fminf(ca.b + cb.b, 1)};
    }
    case 3: {   // respiration : tout l'anneau "respire" en dégradé arc-en-ciel lent
        float breath = 0.15f + 0.85f * (0.5f - 0.5f * cosf(t * 2.2f));
        return hsv(t * 0.08f + i / (n * 3.0f), 1, breath);
    }
    case 4: {   // étincelles multicolores aléatoires (ciel étoilé)
        return hsv(s_spark_hue[i], 0.85f, s_spark[i]);
    }
    case 5: {   // théâtre : une LED sur trois allumée, qui avance, couleurs qui défilent
        int step = (int)(t * 8.0f);
        float v = ((i + step) % 3 == 0) ? 1.0f : 0.06f;
        return hsv(t * 0.2f + i / n * 0.5f, 1, v);
    }
    default: {  // plasma : superposition de vagues sinusoïdales de teintes
        float w = sinf(i * 0.9f + t * 2.0f) + sinf(i * 0.4f - t * 1.3f);
        return hsv(t * 0.05f + w * 0.25f, 1, 0.55f + 0.45f * sinf(i * 0.7f + t * 3.0f));
    }
    }
}

static uint8_t to_led(float c)   // 0..1 -> 0..LED_MAX avec correction gamma (couleurs plus franches)
{
    if (c <= 0) return 0;
    if (c > 1) c = 1;
    return (uint8_t)(c * c * LED_MAX + 0.5f);
}

static void encoder_led_task(void *arg)
{
    if (ws2812_init(LED_PIN, LED_COUNT) != ESP_OK) {
        ESP_LOGE(TAG, "ws2812_init a échoué, animation désactivée");
        vTaskDelete(NULL);
    }

    // mt8901_init() est appelé par screen_init() (tâche LVGL) : on attend qu'il ait démarré
    vTaskDelay(pdMS_TO_TICKS(1500));

    int16_t last = mt8901_get_count();
    int dir = 1;          // dernier sens de rotation (couleur)
    int flash = 0;        // intensité du flash blanc (appui)
    int last_btn = 1;

    // ⚫ 24/10/2026 : état du chenillard d'inactivité
    uint32_t idle = 0;    // nombre d'images sans action sur l'encodeur
    uint32_t show_t = 0;  // images écoulées depuis le début du chenillard
    float show_mix = 0;   // 0 = interactif, 1 = chenillard (fondu entre les deux)

    for (;;) {
        int16_t now = mt8901_get_count();
        int16_t diff = now - last;

        // estompe la traînée
        for (int i = 0; i < LED_COUNT; i++) {
            s_level[i] = (uint8_t)((s_level[i] * FADE_NUM) / 100);
        }

        if (diff != 0) {
            dir = diff > 0 ? 1 : -1;
            last = now;
        }
        // position de la tête = compteur modulo nombre de LED (toujours entre 0 et LED_COUNT-1)
        int pos = now % LED_COUNT;
        if (pos < 0) pos += LED_COUNT;
#if CONFIG_APP_ENCODER_LED_REVERSE
        pos = (LED_COUNT - pos) % LED_COUNT;
#endif
        if (diff != 0) {
            s_level[pos] = 255;
        }

        // appui sur le bouton (actif bas) -> flash blanc
        int btn = gpio_get_level(BTN_PIN);
        if (btn == 0 && last_btn == 1) {
            flash = 255;
        }
        last_btn = btn;

        // ⚫ 24/10/2026 : toute action (rotation, bouton enfoncé) réveille le mode interactif
        if (diff != 0 || btn == 0) {
            idle = 0;
            show_mix = 0;       // coupure immédiate du chenillard : la main revient à l'utilisateur
        } else if (idle < IDLE_FRAMES) {
            idle++;
            if (idle == IDLE_FRAMES) show_t = 0;   // départ du chenillard à l'effet n°0
        }
        bool show_on = (idle >= IDLE_FRAMES);
        if (show_on && show_mix < 1) show_mix += 1.0f / SHOW_FADEIN_FRAMES;
        if (show_mix > 1) show_mix = 1;

        // ⚫ 24/10/2026 : effet courant + fondu enchaîné vers le suivant
        float t = show_t * FRAME_MS / 1000.0f;
        int fx = (show_t / SHOW_EFFECT_FRAMES) % SHOW_EFFECTS;
        int fx_next = (fx + 1) % SHOW_EFFECTS;
        uint32_t in_fx = show_t % SHOW_EFFECT_FRAMES;
        float xfade = 0;
        if (in_fx > SHOW_EFFECT_FRAMES - SHOW_XFADE_FRAMES) {
            xfade = (float)(in_fx - (SHOW_EFFECT_FRAMES - SHOW_XFADE_FRAMES)) / SHOW_XFADE_FRAMES;
        }
        if (show_on) {
            show_t++;
            // étincelles : décroissance + nouvelles étincelles au hasard
            for (int i = 0; i < LED_COUNT; i++) {
                s_spark[i] *= 0.90f;
                if ((esp_random() % 1000) < 45) {
                    s_spark[i] = 1.0f;
                    s_spark_hue[i] = (esp_random() % 1000) / 1000.0f;
                }
            }
        }

        for (int i = 0; i < LED_COUNT; i++) {
            uint32_t l = (uint32_t)s_level[i] * LED_MAX / 255;
            uint32_t w = (uint32_t)flash * LED_MAX / 255;
            uint8_t r, g, b;
            if (dir > 0) { r = 0;           g = l * 3 / 4; b = l;     }   // cyan
            else         { r = l;           g = l * 2 / 5; b = 0;     }   // orange
            r = (r + w > 255) ? 255 : r + w;
            g = (g + w > 255) ? 255 : g + w;
            b = (b + w > 255) ? 255 : b + w;

            // ⚫ 24/10/2026 : mélange avec le chenillard (show_mix = 0 en usage interactif)
            if (show_mix > 0) {
#if CONFIG_APP_ENCODER_LED_REVERSE
                int li = (LED_COUNT - i) % LED_COUNT;
#else
                int li = i;
#endif
                rgbf_t a = show_effect(fx, li, t), c = a;
                if (xfade > 0) {
                    rgbf_t n2 = show_effect(fx_next, li, t);
                    c.r = a.r + (n2.r - a.r) * xfade;
                    c.g = a.g + (n2.g - a.g) * xfade;
                    c.b = a.b + (n2.b - a.b) * xfade;
                }
                r = (uint8_t)(r * (1 - show_mix) + to_led(c.r) * show_mix);
                g = (uint8_t)(g * (1 - show_mix) + to_led(c.g) * show_mix);
                b = (uint8_t)(b * (1 - show_mix) + to_led(c.b) * show_mix);
            }
            ws2812_set(i, r, g, b);
        }
        flash = (flash * 85) / 100;

        ws2812_show();
        vTaskDelay(pdMS_TO_TICKS(FRAME_MS));
    }
}

void encoder_led_start(void)
{
    memset(s_level, 0, sizeof(s_level));
    // xTaskCreatePinnedToCore(encoder_led_task, "enc_led", 3 * 1024, NULL, 3, NULL, 0);
    // ⚫ 24/10/2026 : pile 4 Ko (calculs flottants du chenillard)
    xTaskCreatePinnedToCore(encoder_led_task, "enc_led", 4 * 1024, NULL, 3, NULL, 0);
}

#else

void encoder_led_start(void) {}

#endif
