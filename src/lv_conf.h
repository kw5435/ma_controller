/**
 * lv_conf.h  — LVGL 8.3 configuration for MA Controller
 * Place in src/ so -Isrc makes it discoverable.
 */
#if 1 /* Set this to "1" to enable content */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ---- Color depth ------------------------------------------------- */
#define LV_COLOR_DEPTH 16          /* ILI9341 = RGB565              */
#define LV_COLOR_16_SWAP 1         /* LovyanGFX wants swapped bytes */

/* ---- Memory -------------------------------------------------------- */
/* 32 KB internal heap for LVGL (safe without PSRAM) */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE   (32U * 1024U)

/* ---- HAL ----------------------------------------------------------- */
#define LV_TICK_CUSTOM 0
#define LV_DPI_DEF     130

/* ---- Drawing ------------------------------------------------------- */
#define LV_DRAW_COMPLEX 1
#define LV_SHADOW_CACHE_SIZE 0

/* ---- Features ------------------------------------------------------ */
#define LV_USE_ANIMATION 1
#define LV_USE_SNAPSHOT  0

/* ---- Widget enable ------------------------------------------------- */
#define LV_USE_ARC       1
#define LV_USE_BAR       1
#define LV_USE_BTN       1
#define LV_USE_BTNMATRIX 0
#define LV_USE_CANVAS    0
#define LV_USE_CHECKBOX  0
#define LV_USE_DROPDOWN  1
#define LV_USE_IMG       1
#define LV_USE_LABEL     1
#define LV_USE_LINE      0
#define LV_USE_ROLLER    0
#define LV_USE_SLIDER    1
#define LV_USE_SWITCH    1
#define LV_USE_TEXTAREA  1
#define LV_USE_TABLE     0
#define LV_USE_TABVIEW   0
#define LV_USE_TILEVIEW  1
#define LV_USE_WIN       0
#define LV_USE_SPAN      0
#define LV_USE_MSGBOX    1
#define LV_USE_SPINBOX   0
#define LV_USE_SPINNER   1
#define LV_USE_LIST      1
#define LV_USE_METER     0
#define LV_USE_KEYBOARD  1
#define LV_KEYBOARD_DEF_MODE LV_KEYBOARD_MODE_TEXT_LOWER

/* ---- Extra themes / layouts --------------------------------------- */
#define LV_USE_FLEX     1
#define LV_USE_GRID     0

/* ---- Themes ------------------------------------------------------- */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 1

/* ---- Fonts -------------------------------------------------------- */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT       &lv_font_montserrat_14

/* ---- Logging ------------------------------------------------------ */
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF   1

/* ---- Assert ------------------------------------------------------- */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* ---- Image decoders ---------------------------------------------- */
#define LV_IMG_CF_INDEXED 1

#endif /* LV_CONF_H */
#endif /* Enable content */
