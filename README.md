# MA Controller — ESP32 CYD Web Flasher

Ein ESP32-Firmware-Projekt für das **Cheap Yellow Display (CYD / ESP32-2432S028R)**, das als Hardware-Controller für [Music Assistant](https://music-assistant.io/) dient. Steuere Wiedergabe, Lautstärke und wähle Player direkt über das Touchdisplay — ohne App, ohne Cloud.

---

## Screenshots

### Player-Ansicht
![Player Screen](docs/screenshots/player_screen.png)

> Zeigt Titel, Künstler, Album-Art, Fortschrittsbalken und Steuerknöpfe (Zurück / Play-Pause / Weiter / Lautstärke).

### Einstellungen
![Settings Screen](docs/screenshots/settings_screen.png)

> WLAN-SSID, Passwort, Music-Assistant-IP, Port, API-Token und Player-ID — alle Werte werden im nichtflüchtigen Speicher (NVS) abgelegt.

### Setup-Portal
![Captive Portal](docs/screenshots/captive_portal.png)

> Beim ersten Start öffnet der Controller einen eigenen WLAN-Accesspoint (`MA-Controller`). Nach Verbindung damit öffnet sich automatisch das Konfigurationsportal unter `192.168.4.1`.

---

## Features

- **Touchdisplay-UI** mit LVGL 8 — flüssige Animationen, Dark-Theme
- **WebSocket-Echtzeit-Updates** vom Music-Assistant-Server
- **REST-Fallback-Polling** wenn WebSocket getrennt ist
- **Captive Portal** für einfache Erstkonfiguration
- **NVS-Speicher** — Einstellungen bleiben nach Neustart erhalten
- **RGB-Status-LED** (Rot = verbindet, Grün = verbunden, Blau = Portal aktiv)
- **Web Flasher** — Flash direkt aus dem Browser, kein USB-Treiber nötig

---

## Hardware

| Komponente | Details |
|-----------|---------|
| Board | ESP32-2432S028R ("Cheap Yellow Display") |
| Display | ILI9341 2.8" SPI, 240×320px |
| Touch | XPT2046 resistiv (teilt SPI-Bus mit Display) |
| Backlight | GPIO 21, PWM-dimmbar |
| Status-LED | RGB, active LOW (GPIO 4/16/17) |
| Flash | 4 MB |

---

## Firmware flashen (Web Flasher)

Der einfachste Weg — direkt im Browser, kein Treiber nötig:

**[➜ Web Flasher öffnen](https://kw5435.github.io/ma_controller/)**

1. ESP32 per USB anschließen
2. Link oben öffnen (Chrome oder Edge, kein Firefox)
3. „Install" klicken → COM-Port auswählen → fertig

---

## Manuell flashen (esptool)

```bash
pip install esptool

esptool.py --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x0 firmware-merged.bin
```

Die `firmware-merged.bin` findest du unter [Releases](https://github.com/kw5435/ma_controller/releases).

---

## Lokaler Build (PlatformIO)

```bash
# Abhängigkeiten installieren
pip install platformio

# Firmware bauen
pio run -e cyd_esp32

# Auf ESP32 flashen (USB)
pio run -e cyd_esp32 -t upload
```

Ausgabe: `.pio/build/cyd_esp32/firmware.bin`

---

## Ersteinrichtung

1. ESP32 einschalten → WLAN `MA-Controller` erscheint
2. Mit `MA-Controller` verbinden
3. Browser öffnet `192.168.4.1` automatisch (oder manuell aufrufen)
4. WLAN-Daten + Music-Assistant-Server eintragen
5. Speichern → Controller verbindet sich und zeigt Player-UI

---

## Konfigurationsparameter

| Feld | Beschreibung | Standard |
|------|-------------|---------|
| WLAN SSID | Dein Heimnetz | — |
| WLAN Passwort | — | — |
| MA Server IP | IP deines Music-Assistant-Hosts | — |
| MA Port | API-Port | `8095` |
| API Token | Aus den MA-Einstellungen | — |
| Player ID | Leer = automatisch erster Player | — |

---

## Projektstruktur

```
ma_controller/
├── src/
│   ├── main.cpp              # Bootreihenfolge & Hauptschleife
│   ├── AppConfig.h           # Pin-Definitionen, Konstanten
│   ├── lv_conf.h             # LVGL-Konfiguration
│   ├── api/                  # REST- & WebSocket-Client
│   ├── display/              # LovyanGFX + LVGL Init
│   ├── network/              # WiFi + Captive Portal
│   ├── state/                # PlayerState (lokaler Zustandsspeicher)
│   ├── storage/              # NVS-Persistenz
│   └── ui/                   # LVGL-Screens (Player, Settings, Loading)
├── web-flasher/
│   ├── index.html            # ESP Web Tools Flash-UI
│   ├── manifest.json.tmpl    # Wird durch CI zu manifest.json
│   └── firmware/             # Hier landet firmware-merged.bin nach Build
├── .github/workflows/
│   └── build.yml             # CI: Build → merge → Deploy GitHub Pages
├── platformio.ini
└── merge_firmware.py         # PlatformIO Post-Build: 4 Binaries → 1 .bin
```

---

## CI/CD

Bei jedem Push auf `main` oder `master`:

1. PlatformIO baut die Firmware
2. esptool merged Bootloader + Partition Table + App zu `firmware-merged.bin`
3. Web Flasher wird auf **GitHub Pages** deployed

Bei einem Git-Tag (`v*`) wird zusätzlich ein **GitHub Release** mit der Binary erstellt.

---

## Lizenz

MIT License — Details in [LICENSE](LICENSE)
