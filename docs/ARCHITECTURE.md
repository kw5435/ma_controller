# MA Controller — Architecture Deep Dive

## System Overview

```
┌─────────────────────────────────────────────────────┐
│                    ESP32 (240MHz)                    │
│                                                      │
│  ┌──────────┐    ┌──────────┐    ┌──────────────┐   │
│  │  Storage │    │  State   │    │  WiFiManager │   │
│  │  (NVS)   │    │ (dirty   │    │  + captive   │   │
│  └────┬─────┘    │  flags)  │    │  portal      │   │
│       │          └────┬─────┘    └──────┬───────┘   │
│       │               │                 │           │
│  ┌────▼─────────────────────────────────▼───────┐   │
│  │                  main.cpp                     │   │
│  │         (orchestrates boot + loop)            │   │
│  └────┬──────────────┬────────────┬─────────────┘   │
│       │              │            │                  │
│  ┌────▼────┐   ┌──────▼──────┐  ┌▼───────────────┐  │
│  │  REST   │   │  WebSocket  │  │   UIManager    │  │
│  │ Client  │   │   Client    │  │  (screen ctrl) │  │
│  └────┬────┘   └──────┬──────┘  └┬──────────────┘  │
│       │               │          │                  │
│       └───────────────┘    ┌─────┼──────────────┐   │
│          sends events to   │  PlayerScreen      │   │
│          PlayerState       │  SettingsScreen    │   │
│                            └────────────────────┘   │
│                                      │               │
│                             ┌────────▼────────┐      │
│                             │  LovyanGFX/LVGL │      │
│                             │  ILI9341 + XPT  │      │
│                             └─────────────────┘      │
└─────────────────────────────────────────────────────┘
               │  WiFi
               ▼
  ┌────────────────────────┐
  │  Music Assistant Server│
  │  REST  ws://ip:8095    │
  └────────────────────────┘
```

---

## Module Responsibilities

### `AppConfig.h`
Central constants file. All pin numbers, timing values, NVS keys, colour
palette, and log macros live here. Nothing else is hardcoded.

### `Storage` (NVS)
Wraps ESP32 `Preferences` (Flash NVS). Stores WiFi credentials, MA server
IP/port/token, selected player ID, and display brightness. Survives reboot
and power cycles. Granular savers avoid full rewrites.

### `PlayerState` (State machine)
The **single source of truth** for what the UI displays. Uses a dirty-flag
pattern: every setter marks which field changed. `UIManager` reads and clears
flags every `UI_UPDATE_INTERVAL_MS`, updating only the LVGL widgets that
actually changed. Between server updates, `tick()` advances `elapsedTime`
locally for smooth progress animation.

### `WiFiManager`
Thin wrapper around Arduino WiFi. Handles:
- STA connect with timeout callback
- Auto-reconnect monitoring in `loop()`
- AP + DNS captive portal for first-boot setup

### `RestClient`
Synchronous HTTP (HTTPClient). Used for:
- Initial player list fetch
- Full player state fetch on reconnect
- All command POSTs (play/pause/next/prev/volume)
- Album art JPEG fetch

All calls are guarded by a `WiFi.isConnected()` check. Responses are parsed
with `ArduinoJson` into `PlayerSnapshot` structs.

### `WebSocketClient`
Real-time event stream from MA. On connect sends `{"command":"start_listening"}`.
Dispatches incoming events to `PlayerState` directly. Pings every 30s.
Automatically reconnects if dropped. The WS event handler is a static trampoline
because the WebSockets library requires a C-style function pointer.

### `UIManager`
Coordinator between the network/state layer and the LVGL screens. Owns screen
instances, handles screen transitions with LVGL animations, and drives the
partial-update loop. No LVGL object pointers leak into non-UI code.

### `PlayerScreen`
Full-screen player UI. All LVGL widgets are created once and updated in-place
via setters. Album art decoding uses `TJpg_Decoder` to render JPEG bytes into
an `lv_canvas_t`. Long track titles use `LV_LABEL_LONG_SCROLL_CIRCULAR`.

### `SettingsScreen`
Scrollable config form with a shared `lv_keyboard` that slides up when any
`lv_textarea` gains focus. Saves to NVS and reboots on "Save & Connect".
Auto-fetches player list from MA for the selector widget.

---

## Data Flow

```
MA WebSocket ──► WebSocketClient._handleMessage()
                    │
                    ▼
              PlayerState.applySnapshot()  ← sets dirty flags
                    │
              [every 50ms]
                    ▼
              UIManager.update()
                    │
                    ├─ dirty.playState  → PlayerScreen.updatePlayState()
                    ├─ dirty.trackInfo  → PlayerScreen.updateTrackInfo()
                    ├─ dirty.albumArt   → PlayerScreen.updateAlbumArt()
                    ├─ dirty.volume     → PlayerScreen.updateVolume()
                    └─ dirty.progress   → PlayerScreen.updateProgress()
                                               │
                                         lv_label_set_text()
                                         lv_bar_set_value()
                                         lv_slider_set_value()
                                               │
                                         LVGL renders only
                                         changed regions
```

Touch events flow in the reverse direction:
```
User tap → XPT2046 IRQ → LVGL indev callback → lv_event_t
         → _onPlayPause() → RestClient.cmdPause() → HTTP POST
         → PlayerState.applyPlayState(PAUSED)   ← optimistic update
```

---

## UI Partial Update Strategy

LVGL 8 already does region-based dirty tracking internally (only flushes
changed areas to the display). The additional dirty-flag layer above it
prevents even calling `lv_label_set_text()` unless the data changed, which
avoids LVGL re-measuring and re-rendering text that hasn't moved.

Key techniques:
- **Double frame buffers** (`buf1` / `buf2`, each 240×32 pixels) so DMA
  transfers one buffer while LVGL renders into the other.
- **`LV_ANIM_ON`** on bar/slider updates for smooth interpolation.
- **`LV_LABEL_LONG_SCROLL_CIRCULAR`** for track title — LVGL handles the
  scroll animation internally with no main-loop involvement.
- Album art is decoded once and cached in an `lv_canvas_t`; re-requested
  only when `imageUrl` changes (checked in `PlayerState.applySnapshot()`).

---

## Memory Layout (4 MB Flash, 320 KB SRAM)

```
Flash:
  Bootloader          ~32 KB
  Partition table      ~4 KB
  NVS partition       ~20 KB  (min_spiffs.csv)
  OTA data             ~8 KB
  App (code+rodata)  ~900 KB

SRAM at runtime:
  FreeRTOS stacks    ~32 KB
  Arduino loop task  ~8 KB  (CONFIG_ARDUINO_LOOP_STACK_SIZE)
  WiFi/TCP driver    ~60 KB  (Espressif blobs)
  LVGL heap          ~32 KB  (LV_MEM_SIZE)
  LVGL frame bufs    ~15 KB  (2 × 240×32 × 2 bytes)
  Art canvas buf      ~13 KB  (80×80 × 2 bytes)
  ArduinoJson docs   ~8 KB   (per parse, stack)
  App globals + heap ~50 KB
```

Remaining free heap at runtime: ~100 KB — sufficient for connection buffers
and JPEG stream.
