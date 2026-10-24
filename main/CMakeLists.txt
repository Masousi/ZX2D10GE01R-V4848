idf_component_get_property(LVGL_DIR lvgl COMPONENT_DIR)

# ⚫ MODIFIÉ (24/10/2026) : ajout de toutes les démos LVGL (le composant lvgl 8.3.3 ne compile que src/).
# Chaque fichier de démo est protégé par son #if LV_USE_DEMO_xxx : une démo désactivée compile vide.
file(GLOB LVGL_DEMOS
    ${LVGL_DIR}/demos/benchmark
    ${LVGL_DIR}/demos/benchmark/assets
    ${LVGL_DIR}/demos/widgets
    ${LVGL_DIR}/demos/widgets/assets
    ${LVGL_DIR}/demos/keypad_encoder
    ${LVGL_DIR}/demos/stress
    ${LVGL_DIR}/demos/music
    ${LVGL_DIR}/demos/music/assets
)

idf_component_register(
    SRC_DIRS ${DIRS} ${LVGL_DEMOS} "."
    INCLUDE_DIRS "." ${LVGL_DEMOS} ${DIRS}
    # REQUIRES esp_lcd driver lvgl esp_timer button mt8901
    # ⚫ MODIFIÉ (24/10/2026) : + ws2812 (LED de l'encodeur)
    REQUIRES esp_lcd driver lvgl esp_timer button mt8901 ws2812
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")

