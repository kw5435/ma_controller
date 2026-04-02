#pragma once
/**
 * display/DisplaySetup.h
 *
 * Configures LovyanGFX for CYD ILI9341 + XPT2046 touch
 * and binds the display/touch to LVGL's HAL callbacks.
 */
#include <Arduino.h>
#include <lvgl.h>

class DisplaySetup {
public:
    static void init();
    static void setBrightness(uint8_t val);   // 0-255

    // LVGL flush / read callbacks (called by LVGL internally)
    static void lvglFlushCb(lv_disp_drv_t* drv,
                            const lv_area_t* area,
                            lv_color_t* colorMap);
    static void lvglTouchCb(lv_indev_drv_t* drv,
                            lv_indev_data_t* data);
};
