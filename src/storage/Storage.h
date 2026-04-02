#pragma once
/**
 * storage/Storage.h + Storage.cpp (combined for brevity)
 *
 * Wraps ESP32 Preferences (NVS) for persistent configuration.
 * All keys use the NVS_NAMESPACE defined in AppConfig.h.
 */
#include <Arduino.h>

struct AppSettings {
    String ssid;
    String password;
    String serverIP;
    uint16_t serverPort  = 8095;
    String apiToken;       // empty = no auth
    String playerId;       // selected MA player
    uint8_t brightness   = 200;  // 0-255
    bool   configured    = false;
};

class Storage {
public:
    bool        begin();
    AppSettings loadSettings();
    bool        saveSettings(const AppSettings& s);
    void        clearAll();

    // Granular savers (avoid full rewrite on single-field change)
    bool savePlayerId(const String& id);
    bool saveBrightness(uint8_t v);
};
