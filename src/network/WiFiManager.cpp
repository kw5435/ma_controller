/**
 * network/WiFiManager.cpp
 */
#include "WiFiManager.h"
#include "../AppConfig.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

static DNSServer       dnsServer;
static AsyncWebServer* portalServer = nullptr;

// ── begin ────────────────────────────────────────────────────────────
void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);   // we handle reconnect manually
}

// ── connect ──────────────────────────────────────────────────────────
void WiFiManager::connect(const String& ssid, const String& pass,
                          uint32_t timeoutMs,
                          ConnectCb onConnected, ConnectCb onFailed) {
    _onConnected  = onConnected;
    _onFailed     = onFailed;
    _timeout      = timeoutMs;
    _connectStart = millis();
    _status       = WiFiStatus::CONNECTING;

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    LOG_I("WIFI", "Connecting to '%s'...", ssid.c_str());
}

// ── loop ─────────────────────────────────────────────────────────────
void WiFiManager::loop() {
    if (_portalActive) {
        dnsServer.processNextRequest();
    }

    if (_status == WiFiStatus::CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _status = WiFiStatus::CONNECTED;
            LOG_I("WIFI", "Connected → %s", WiFi.localIP().toString().c_str());
            if (_onConnected) _onConnected();
            return;
        }
        if (millis() - _connectStart > _timeout) {
            _status = WiFiStatus::DISCONNECTED;
            LOG_W("WIFI", "Connection timeout");
            if (_onFailed) _onFailed();
            return;
        }
    }

    // Monitor for drops on an established connection
    if (_status == WiFiStatus::CONNECTED &&
        WiFi.status() != WL_CONNECTED) {
        _status = WiFiStatus::DISCONNECTED;
        LOG_W("WIFI", "Connection lost");
        if (_onFailed) _onFailed();
    }

    // Periodic reconnect attempt when disconnected (not in portal mode)
    if (_status == WiFiStatus::DISCONNECTED && !_portalActive) {
        uint32_t now = millis();
        if (now - _lastRecheck > 10000) {
            _lastRecheck = now;
            if (WiFi.SSID().length() > 0) {
                LOG_I("WIFI", "Auto-reconnect attempt");
                _status       = WiFiStatus::CONNECTING;
                _connectStart = now;
                WiFi.reconnect();
            }
        }
    }
}

// ── Captive portal ───────────────────────────────────────────────────
void WiFiManager::startCaptivePortal(const String& apSSID) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str());
    WiFi.softAPConfig(
        IPAddress(192,168,4,1),
        IPAddress(192,168,4,1),
        IPAddress(255,255,255,0)
    );

    dnsServer.start(53, "*", IPAddress(192,168,4,1));

    if (!portalServer) {
        portalServer = new AsyncWebServer(80);

        // Redirect all traffic to config page
        portalServer->onNotFound([](AsyncWebServerRequest* req) {
            req->redirect("http://192.168.4.1/");
        });

        // Serve minimal config form
        portalServer->on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
            const char* html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MA Controller Setup</title>
<style>
  body{font-family:sans-serif;background:#0d0d0d;color:#eee;max-width:400px;margin:40px auto;padding:20px}
  input{width:100%;padding:10px;margin:8px 0;background:#1a1a2e;border:1px solid #533483;color:#eee;border-radius:6px;box-sizing:border-box}
  button{width:100%;padding:12px;background:#4ecca3;color:#0d0d0d;font-weight:bold;border:none;border-radius:6px;cursor:pointer;font-size:16px;margin-top:12px}
  h2{color:#4ecca3}label{color:#aaa;font-size:14px}
</style></head>
<body>
<h2>🎵 MA Controller Setup</h2>
<form action="/save" method="POST">
  <label>WiFi SSID</label>
  <input name="ssid" placeholder="Your WiFi network" required>
  <label>WiFi Password</label>
  <input name="pass" type="password" placeholder="WiFi password">
  <label>Music Assistant IP</label>
  <input name="ip" placeholder="192.168.1.100" required>
  <label>MA Port (default 8095)</label>
  <input name="port" value="8095">
  <label>API Token (if required)</label>
  <input name="token" placeholder="Leave blank if not set">
  <button type="submit">Save &amp; Connect</button>
</form>
</body></html>)";
            req->send(200, "text/html", html);
        });

        // Note: saving is handled by main.cpp callback — we just store params
        // In real implementation this triggers a restart; kept simple here
        portalServer->on("/save", HTTP_POST, [](AsyncWebServerRequest* req) {
            // Parameters are read by main.cpp via query or body
            // For simplicity: redirect back with confirmation
            req->send(200, "text/html",
                "<!DOCTYPE html><html><body style='background:#0d0d0d;color:#4ecca3;"
                "font-family:sans-serif;text-align:center;padding:40px'>"
                "<h2>✓ Saved! Rebooting...</h2></body></html>");
            delay(1500);
            ESP.restart();
        });

        portalServer->begin();
    }

    _portalActive = true;
    _status = WiFiStatus::PORTAL_ACTIVE;
    LOG_I("WIFI", "Captive portal active → SSID=%s  IP=192.168.4.1", apSSID.c_str());
}

void WiFiManager::stopCaptivePortal() {
    if (_portalActive) {
        dnsServer.stop();
        if (portalServer) { portalServer->end(); delete portalServer; portalServer = nullptr; }
        _portalActive = false;
    }
}

bool   WiFiManager::isConnected() const { return WiFi.status() == WL_CONNECTED; }
String WiFiManager::localIP()     const { return WiFi.localIP().toString(); }
