#pragma once
/**
 * AppConfig.h — CYD pin map, MA API constants, timing
 *
 * CYD = "Cheap Yellow Display" ESP32-2432S028 (USB-C variant)
 *   Display  : ST7789V SPI on VSPI (GPIO 14/13/12/15/2) — portrait 240×320
 *   Touch    : XPT2046 Software-SPI (GPIO 25/32/39/33/36) — separate bus
 *   Backlight: GPIO 21 (active HIGH, PWM-capable)
 */

// ── Display / SPI pins ──────────────────────────────────────────────
#define PIN_TFT_MOSI  13
#define PIN_TFT_MISO  12
#define PIN_TFT_CLK   14
#define PIN_TFT_CS    15
#define PIN_TFT_DC     2
#define PIN_TFT_RST   -1   // tied to EN/RST line
#define PIN_TFT_BL    21   // PWM backlight

// ── Touch (XPT2046, separate VSPI bus) ──────────────────────────────
#define PIN_TOUCH_CS   33
#define PIN_TOUCH_IRQ  36   // input-only pin
#define PIN_TOUCH_CLK  25
#define PIN_TOUCH_MOSI 32
#define PIN_TOUCH_MISO 39

// ── RGB Status LED (active LOW) ──────────────────────────────────────
#define PIN_LED_R      4
#define PIN_LED_G     16
#define PIN_LED_B     17

// ── Display geometry ────────────────────────────────────────────────
#define DISP_W  240
#define DISP_H  320

// ── LVGL frame buffer ───────────────────────────────────────────────
// Two buffers of 1/10 screen height for partial refresh
#define LVGL_BUF_LINES  (DISP_H / 10)

// ── Music Assistant API ──────────────────────────────────────────────
#define MA_DEFAULT_PORT    8095
#define MA_WS_PATH         "/api/ws"
#define MA_REST_BASE       "/api"
#define MA_AUTH_HEADER     "x-api-key"

// REST command paths (format: MA_REST_BASE "/players/{id}/cmd/{cmd}")
#define MA_CMD_PLAY        "play"
#define MA_CMD_PAUSE       "pause"
#define MA_CMD_NEXT        "next"
#define MA_CMD_PREVIOUS    "previous"
#define MA_CMD_STOP        "stop"
#define MA_CMD_VOL_SET     "volume_set"

// ── Timing ──────────────────────────────────────────────────────────
#define WIFI_TIMEOUT_MS      15000
#define WS_RECONNECT_MS       5000
#define FALLBACK_POLL_MS     30000   // REST poll when WS disconnected
#define UI_UPDATE_INTERVAL_MS   50   // min ms between UI refreshes
#define PROGRESS_UPDATE_MS    1000   // local seek bar tick

// ── NVS Storage keys ────────────────────────────────────────────────
#define NVS_NAMESPACE    "ma_ctrl"
#define NVS_SSID         "ssid"
#define NVS_PASS         "wpass"
#define NVS_MA_IP        "ma_ip"
#define NVS_MA_PORT      "ma_port"
#define NVS_MA_TOKEN     "ma_token"
#define NVS_PLAYER_ID    "player_id"
#define NVS_BRIGHTNESS   "bright"

// ── Captive portal ──────────────────────────────────────────────────
#define PORTAL_SSID   "MA-Controller"
#define PORTAL_IP     "192.168.4.1"

// ── UI Colors (LVGL hex) ────────────────────────────────────────────
#define COL_BG          0x0D0D0D   // near-black
#define COL_SURFACE     0x1A1A2E   // dark navy
#define COL_CARD        0x16213E
#define COL_ACCENT      0x0F3460
#define COL_HIGHLIGHT   0x533483   // purple accent
#define COL_PLAY_BTN    0x4ECCA3   // teal
#define COL_TEXT_PRI    0xEEEEEE
#define COL_TEXT_SEC    0xAAAAAA
#define COL_DISABLED    0x555555

// ── Structured log macros ───────────────────────────────────────────
#define LOG_TAG(tag, fmt, ...) \
    Serial.printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)

#define LOG_I(tag, fmt, ...) LOG_TAG(tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) LOG_TAG("WARN/" tag, fmt, ##__VA_ARGS__)
#define LOG_E(tag, fmt, ...) LOG_TAG("ERR/" tag,  fmt, ##__VA_ARGS__)
