// ⚫ AJOUTÉ (24/10/2026) : API de l'anneau de LED de l'encodeur (main/encoder_led.c),
// utilisée par main.c (démarrage) et par la démo "LED animations" (main/demo_led.c).
#pragma once

// Animations, dans l'ordre de show_effect() :
// 0 arc-en-ciel tournant, 1 comète arc-en-ciel, 2 comètes croisées, 3 respiration,
// 4 étincelles, 5 théâtre, 6 plasma, 7 Random (LED, couleurs et durées aléatoires)
#define ENC_LED_FX_RANDOM    7
#define ENC_LED_FX_COUNT     8
#define ENC_LED_MODE_AUTO  (-1)    // enchaînement des 7 premiers effets (comportement par défaut, autres démos)

// Lance la tâche d'animation (appelé une fois par main.c).
void encoder_led_start(void);

// Choisit l'animation : 0..7 (jouée en continu) ou ENC_LED_MODE_AUTO.
// L'animation démarre immédiatement (sans attendre APP_ENCODER_LED_IDLE_S).
void encoder_led_set_mode(int mode);

// Animation actuellement choisie.
int encoder_led_get_mode(void);
