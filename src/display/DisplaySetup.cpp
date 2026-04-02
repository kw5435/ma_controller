/**
 * display/DisplaySetup.cpp
 *
 * CYD ESP32-2432S028R hardware:
 *   Panel  : ILI9341 (SPI)  320×240 physical, rotated 90° → 240×320 portrait
 *   Touch  : XPT2046 (same SPI bus as display)
 *   BL     : GPIO 21 PWM (active HIGH on most CYD variants)
 */
#include "DisplaySetup.h"
#include "../AppConfig.h"
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <SPI.h>

// ── LovyanGFX class definition ────────────────────────────────────────
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_SPI        _bus;
    lgfx::Light_PWM      _light;
    lgfx::Touch_XPT2046  _touch;

public:
    LGFX() {
        // ── SPI bus ──────────────────────────────────────────────────
        { auto cfg = _bus.config();
          cfg.spi_host   = HSPI_HOST;
          cfg.spi_mode   = 0;
          cfg.freq_write = 40000000;
          cfg.freq_read  =  8000000;
          cfg.pin_sclk   = PIN_TFT_CLK;
          cfg.pin_mosi   = PIN_TFT_MOSI;
          cfg.pin_miso   = PIN_TFT_MISO;
          cfg.pin_dc     = PIN_TFT_DC;
          cfg.dma_channel= 1;
          _bus.config(cfg);
          _panel.setBus(&_bus); }

        // ── Panel ────────────────────────────────────────────────────
        { auto cfg = _panel.config();
          cfg.pin_cs    = PIN_TFT_CS;
          cfg.pin_rst   = PIN_TFT_RST;
          cfg.pin_busy  = -1;
          cfg.panel_width  = 240;
          cfg.panel_height = 320;
          cfg.offset_x     = 0;
          cfg.offset_y     = 0;
          cfg.offset_rotation = 0;
          cfg.dummy_read_pixel = 8;
          cfg.dummy_read_bits  = 1;
          cfg.readable     = true;
          cfg.invert       = false;
          cfg.rgb_order    = false;
          cfg.dlen_16bit   = false;
          cfg.bus_shared   = true;
          _panel.config(cfg); }

        // ── Backlight PWM ────────────────────────────────────────────
        { auto cfg = _light.config();
          cfg.pin_bl     = PIN_TFT_BL;
          cfg.invert     = false;
          cfg.freq       = 44100;
          cfg.pwm_channel= 7;
          _light.config(cfg);
          _panel.setLight(&_light); }

        // ── Touch ────────────────────────────────────────────────────
        { auto cfg = _touch.config();
          cfg.x_min    =  300;
          cfg.x_max    = 3900;
          cfg.y_min    =  300;
          cfg.y_max    = 3900;
          cfg.pin_int  = PIN_TOUCH_IRQ;
          cfg.pin_cs   = PIN_TOUCH_CS;
          cfg.bus_shared = true;
          cfg.offset_rotation = 0;
          _touch.config(cfg);
          _panel.setTouch(&_touch); }

        setPanel(&_panel);
    }
};

// ── Static instances ──────────────────────────────────────────────────
static LGFX               lcd;
static lv_disp_drv_t      dispDrv;
static lv_indev_drv_t     indevDrv;
static lv_disp_draw_buf_t drawBuf;
static lv_color_t         buf1[DISP_W * LVGL_BUF_LINES];
static lv_color_t         buf2[DISP_W * LVGL_BUF_LINES];

// ── LVGL flush callback ───────────────────────────────────────────────
void DisplaySetup::lvglFlushCb(lv_disp_drv_t* drv,
                               const lv_area_t* area,
                               lv_color_t* colorMap) {
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((lgfx::rgb565_t*)colorMap, w * h, true);
    lcd.endWrite();
    lv_disp_flush_ready(drv);
}

// ── LVGL touch read callback ──────────────────────────────────────────
void DisplaySetup::lvglTouchCb(lv_indev_drv_t* drv,
                               lv_indev_data_t* data) {
    uint16_t tx, ty;
    bool pressed = lcd.getTouch(&tx, &ty);
    if (pressed) {
        data->point.x = tx;
        data->point.y = ty;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state   = LV_INDEV_STATE_RELEASED;
    }
}

// ── init ─────────────────────────────────────────────────────────────
void DisplaySetup::init() {
    LOG_I("DISP", "Initialising display...");

    // Free HSPI bus if already claimed by Arduino SPI library
    spi_bus_free(HSPI_HOST);

    lcd.init();
    lcd.setRotation(0);      // portrait 240×320
    lcd.setBrightness(200);
    lcd.fillScreen(TFT_BLACK);

    // ── LVGL init ─────────────────────────────────────────────────
    lv_init();

    lv_disp_draw_buf_init(&drawBuf, buf1, buf2,
                          DISP_W * LVGL_BUF_LINES);

    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res  = DISP_W;
    dispDrv.ver_res  = DISP_H;
    dispDrv.flush_cb = lvglFlushCb;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    lv_indev_drv_init(&indevDrv);
    indevDrv.type    = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = lvglTouchCb;
    lv_indev_drv_register(&indevDrv);

    // Apply dark theme
    lv_theme_t* th = lv_theme_default_init(
        lv_disp_get_default(),
        lv_palette_main(LV_PALETTE_PURPLE),
        lv_palette_main(LV_PALETTE_TEAL),
        true,                   // dark
        &lv_font_montserrat_14
    );
    lv_disp_set_theme(lv_disp_get_default(), th);

    LOG_I("DISP", "LVGL %d.%d.%d ready — %dx%d",
          LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
          DISP_W, DISP_H);
}

void DisplaySetup::setBrightness(uint8_t val) {
    lcd.setBrightness(val);
}
