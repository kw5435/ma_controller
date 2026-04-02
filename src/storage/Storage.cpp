/**
 * storage/Storage.cpp
 */
#include "Storage.h"
#include "../AppConfig.h"
#include <Preferences.h>

static Preferences prefs;

bool Storage::begin() {
    bool ok = prefs.begin(NVS_NAMESPACE, false);
    LOG_I("NVS", "Preferences open: %s", ok ? "OK" : "FAIL");
    return ok;
}

AppSettings Storage::loadSettings() {
    AppSettings s;
    s.ssid       = prefs.getString(NVS_SSID,       "");
    s.password   = prefs.getString(NVS_PASS,       "");
    s.serverIP   = prefs.getString(NVS_MA_IP,      "");
    s.serverPort = prefs.getUShort(NVS_MA_PORT,    MA_DEFAULT_PORT);
    s.apiToken   = prefs.getString(NVS_MA_TOKEN,   "");
    s.playerId   = prefs.getString(NVS_PLAYER_ID,  "");
    s.brightness = prefs.getUChar(NVS_BRIGHTNESS,  200);
    s.configured = (!s.ssid.isEmpty() && !s.serverIP.isEmpty());

    LOG_I("NVS", "Loaded — SSID=%s MA=%s:%d player=%s",
          s.ssid.c_str(), s.serverIP.c_str(), s.serverPort,
          s.playerId.c_str());
    return s;
}

bool Storage::saveSettings(const AppSettings& s) {
    prefs.putString(NVS_SSID,      s.ssid);
    prefs.putString(NVS_PASS,      s.password);
    prefs.putString(NVS_MA_IP,     s.serverIP);
    prefs.putUShort(NVS_MA_PORT,   s.serverPort);
    prefs.putString(NVS_MA_TOKEN,  s.apiToken);
    prefs.putString(NVS_PLAYER_ID, s.playerId);
    prefs.putUChar(NVS_BRIGHTNESS, s.brightness);

    LOG_I("NVS", "Settings saved");
    return true;
}

bool Storage::savePlayerId(const String& id) {
    prefs.putString(NVS_PLAYER_ID, id);
    LOG_I("NVS", "Player ID saved: %s", id.c_str());
    return true;
}

bool Storage::saveBrightness(uint8_t v) {
    prefs.putUChar(NVS_BRIGHTNESS, v);
    return true;
}

void Storage::clearAll() {
    prefs.clear();
    LOG_W("NVS", "All settings cleared");
}
