# MA Controller — Setup & Flash Guide

## Hardware

| Component | Spec |
|-----------|------|
| Board | ESP32-2432S028R (CYD) |
| Display | ILI9341, 2.8", 240×320, SPI |
| Touch | XPT2046 resistive, same SPI bus |
| Flash | 4MB |
| RAM | 320 KB SRAM (no PSRAM on base CYD) |

---

## Prerequisites

1. **VS Code** + **PlatformIO extension** (recommended)  
   _Or_: PlatformIO CLI (`pip install platformio`)

2. **USB-Serial driver** for CH340/CP2102 (usually auto-installed on Linux/Mac)

3. **Python 3.8+** (used by PlatformIO build system)

---

## 1 — Clone / Download

```bash
git clone <repo-url> ma_controller
cd ma_controller
```

Or download the ZIP and extract.

---

## 2 — Open in PlatformIO

In VS Code: **File → Open Folder → ma_controller**

PlatformIO auto-detects `platformio.ini` and installs dependencies on first build.

---

## 3 — Build & Flash

```bash
# Build
pio run -e cyd_esp32

# Upload (replace /dev/ttyUSB0 with your port)
pio run -e cyd_esp32 --target upload --upload-port /dev/ttyUSB0

# Windows example
pio run -e cyd_esp32 --target upload --upload-port COM5

# Monitor serial output
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

Or use the PlatformIO sidebar buttons: **Build → Upload → Monitor**

---

## 4 — First-Boot Setup

On first boot (or after factory reset), the device starts in **Setup Mode**:

```
📶  WiFi SSID: MA-Controller
     Open: http://192.168.4.1
```

1. Connect your phone/PC to the `MA-Controller` WiFi network  
2. A captive portal page opens automatically (or navigate to `192.168.4.1`)
3. Fill in:
   - **WiFi SSID & Password** — your home network
   - **Music Assistant IP** — e.g. `192.168.1.105`
   - **MA Port** — default `8095`
   - **API Token** — leave blank unless MA requires auth
4. Tap **Save & Connect** — device reboots and connects

---

## 5 — On-Device Settings

Tap the **⚙ gear icon** (top-right of player screen) at any time to:

- Change WiFi / server settings
- Adjust display brightness
- Select a different MA player
- Factory reset

---

## 6 — Finding Your Music Assistant IP

In Home Assistant:  
**Settings → Add-ons → Music Assistant → Info** → shows the IP

Or check your router's DHCP list for a device named `music-assistant`.

---

## 7 — Player Selection

If your MA server has multiple players (e.g. Chromecast + Spotify Connect):

1. Open Settings screen (⚙)  
2. The **Select Player** list auto-fetches from MA  
3. Tap the player you want this controller to control  
4. Selection is saved to NVS immediately

---

## File Structure

```
ma_controller/
├── platformio.ini          ← Build config + library deps
├── docs/
│   └── api_schema.md       ← MA API reference + JSON examples
└── src/
    ├── lv_conf.h            ← LVGL feature flags
    ├── AppConfig.h          ← Pin map, constants, log macros
    ├── main.cpp             ← Boot + main loop orchestration
    ├── display/
    │   ├── DisplaySetup.h
    │   └── DisplaySetup.cpp ← LovyanGFX + LVGL HAL binding
    ├── storage/
    │   ├── Storage.h
    │   └── Storage.cpp      ← NVS persistent settings
    ├── state/
    │   ├── PlayerState.h
    │   └── PlayerState.cpp  ← Central state + dirty flags
    ├── network/
    │   ├── WiFiManager.h
    │   └── WiFiManager.cpp  ← WiFi + captive portal
    ├── api/
    │   ├── RestClient.h
    │   ├── RestClient.cpp   ← HTTP REST commands + album art
    │   ├── WebSocketClient.h
    │   └── WebSocketClient.cpp ← Real-time WS event processing
    └── ui/
        ├── UIManager.h
        ├── UIManager.cpp    ← Screen coordinator + partial updates
        ├── PlayerScreen.h
        ├── PlayerScreen.cpp ← Main playback UI
        ├── SettingsScreen.h
        └── SettingsScreen.cpp ← Config UI + on-screen keyboard
```

---

## Library Versions (pinned in platformio.ini)

| Library | Version | Purpose |
|---------|---------|---------|
| LovyanGFX | ^1.1.16 | ILI9341 display + XPT2046 touch driver |
| lvgl | ^8.3.11 | UI framework |
| ArduinoJson | ^6.21.5 | JSON parsing |
| WebSockets (links2004) | ^2.4.1 | WebSocket client |
| ESP Async WebServer | ^1.2.3 | Captive portal HTTP server |
| AsyncTCP | ^1.1.1 | Async TCP base for above |
| TJpg_Decoder | ^1.0.8 | JPEG album art decode |

---

## Memory Budget (typical)

| Region | Used | Budget |
|--------|------|--------|
| Flash | ~900 KB | 4 MB |
| SRAM heap at boot | ~160 KB free | 320 KB total |
| LVGL heap | 32 KB | configurable in lv_conf.h |
| LVGL frame buffers | ~15 KB (2× 240×32) | — |
| JPEG art decode | ~6 KB (80×80 RGB565) | — |
| ArduinoJson doc | 4–8 KB per parse | stack/heap |

> ⚠️ If you add large features, reduce `LV_MEM_SIZE` in `lv_conf.h` or
> upgrade to a CYD variant with PSRAM.

---

## Adjusting Touch Calibration

In `DisplaySetup.cpp`, the XPT2046 raw ADC ranges may need tuning for your unit:

```cpp
cfg.x_min = 300;   // raw ADC at x=0
cfg.x_max = 3900;  // raw ADC at x=240
cfg.y_min = 300;
cfg.y_max = 3900;
```

To calibrate: add `Serial.printf("touch raw: %d %d\n", tx, ty);` in `lvglTouchCb`
and tap the four corners while watching serial output.

---

## Serial Log Format

```
[MAIN]   Boot messages, heap reports
[DISP]   Display init
[NVS]    Settings load/save
[WIFI]   Connection status
[REST]   HTTP requests + status codes
[WS]     WebSocket events
[STATE]  Player state transitions
[UI]     Screen transitions
```

Filter level: set `CORE_DEBUG_LEVEL` in `platformio.ini` (3 = info, 4 = debug).

---

## Framework Choice: Arduino vs ESP-IDF

This project uses **Arduino framework** because:

- LovyanGFX and WebSockets libs have first-class Arduino support
- LVGL Arduino integration is well-documented
- Faster iteration; no CMake toolchain required
- ESP32 Arduino runs FreeRTOS under the hood anyway

For a production device, migrating the network layer to ESP-IDF tasks would
give better timing guarantees, but for a single-user touchscreen controller
the Arduino event loop is entirely sufficient.

---

## Customisation Tips

### Change the accent colour
Edit `COL_PLAY_BTN`, `COL_HIGHLIGHT` in `AppConfig.h`.

### Add shuffle / repeat buttons
Add `lv_btn_create` calls in `PlayerScreen::_buildControls()` and wire to
`restClient.cmdShuffleToggle()` (add the REST call in `RestClient`).

### Multiple players / quick-switch
Add a swipe gesture on the player screen to cycle through saved player IDs.

### Screensaver
Add a timer in `UIManager::update()` that dims the backlight after N seconds
of no touch input using `DisplaySetup::setBrightness(20)`.
