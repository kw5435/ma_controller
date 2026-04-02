# Web Flasher Setup Guide

## Wie funktioniert das?

```
Dein Code (GitHub)
       │
       ▼ push to main
GitHub Actions
       │  pio run → firmware.bin
       │  esptool merge_bin → firmware-merged.bin
       ▼
GitHub Pages  →  https://DEIN_USER.github.io/ma_controller/
                         │
                         │  Browser öffnet Seite
                         ▼
                    ESP Web Tools
                         │  Web Serial API
                         ▼
                    ESP32 (CYD)  ✓ geflasht
```

---

## Schritt 1 — Repository auf GitHub anlegen

```bash
cd ma_controller
git init
git add .
git commit -m "Initial commit"

# GitHub Repository erstellen (ohne README, ohne .gitignore)
git remote add origin https://github.com/DEIN_USER/ma_controller.git
git branch -M main
git push -u origin main
```

---

## Schritt 2 — GitHub Pages aktivieren

1. GitHub → Repository → **Settings** → **Pages**
2. Source: **GitHub Actions**  ← wichtig, nicht "Deploy from branch"
3. Speichern

---

## Schritt 3 — Build abwarten

1. GitHub → Repository → **Actions**-Tab
2. Der Workflow "Build & Release Firmware" startet automatisch
3. Grüner Haken = Build erfolgreich (~3–5 Minuten beim ersten Mal)
4. Die Flasher-Seite ist jetzt live unter:
   ```
   https://DEIN_USER.github.io/ma_controller/
   ```

---

## Schritt 4 — ESP32 flashen

1. **Chrome oder Edge** öffnen (kein Firefox, kein Safari)
2. Flasher-URL öffnen
3. ESP32 per USB verbinden
4. **Flash** klicken → Port auswählen → warten (~30 Sekunden)
5. Done ✓

---

## Release erstellen (für stabile Versionen)

```bash
git tag v1.0.0
git push origin v1.0.0
```

→ GitHub Actions baut automatisch und erstellt ein Release mit `firmware-merged.bin` als Download-Anhang.

---

## Lokaler Build (ohne GitHub)

Voraussetzung: PlatformIO installiert

```bash
# Build + automatisches Mergen
pio run -e cyd_esp32

# Ergebnis:
#   .pio/build/cyd_esp32/firmware-merged.bin
#   web-flasher/firmware/firmware-merged.bin

# Flasher lokal starten (Python HTTP Server)
cd web-flasher
python3 -m http.server 8080
# Dann http://localhost:8080 in Chrome öffnen
```

> **Wichtig:** Der lokale Python-Server muss auf Port 80 oder über HTTPS laufen,
> damit Web Serial funktioniert. Alternativ Chrome mit Flag starten:
> ```
> chrome --unsafely-treat-insecure-origin-as-secure=http://localhost:8080
> ```

---

## Treiber

| System  | Chip   | Download |
|---------|--------|----------|
| Windows | CH340  | https://www.wch-ic.com/downloads/CH341SER_EXE.html |
| Windows | CP2102 | https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers |
| macOS   | CH340  | https://www.wch-ic.com/downloads/CH341SER_MAC_ZIP.html |
| Linux   | beide  | kein Treiber nötig (im Kernel) |

---

## Troubleshooting

| Problem | Lösung |
|---------|--------|
| Kein COM-Port sichtbar | Treiber installieren, anderen USB-Kabel probieren |
| "Failed to connect" | BOOT-Taste halten → Flash klicken → loslassen wenn "Connecting..." |
| Build schlägt fehl in CI | Actions-Tab → Fehlermeldung lesen; meist fehlende Lib-Version |
| Seite lädt nicht (GitHub Pages) | Settings → Pages → Source auf "GitHub Actions" setzen |
| esptool.py nicht gefunden (lokal) | `pip install esptool` |

---

## Dateistruktur nach Setup

```
ma_controller/
├── .github/
│   └── workflows/
│       └── build.yml          ← CI: build + deploy
├── web-flasher/
│   ├── index.html             ← Flasher-UI
│   ├── manifest.json.tmpl     ← Vorlage (Version wird eingesetzt)
│   ├── manifest.json          ← generiert von CI
│   ├── .nojekyll              ← GitHub Pages: kein Jekyll
│   └── firmware/
│       ├── README.md
│       └── firmware-merged.bin  ← generiert von CI (nicht committen)
├── src/                       ← Quellcode
├── merge_firmware.py          ← PlatformIO post-build script
├── platformio.ini
└── .gitignore
```
