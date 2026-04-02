#pragma once
/**
 * network/WiFiManager.h
 */
#include <Arduino.h>
#include <functional>

enum class WiFiStatus { DISCONNECTED, CONNECTING, CONNECTED, PORTAL_ACTIVE };

class WiFiManager {
public:
    using ConnectCb = std::function<void()>;

    void begin();
    void connect(const String& ssid, const String& pass,
                 uint32_t timeoutMs,
                 ConnectCb onConnected,
                 ConnectCb onFailed);

    void startCaptivePortal(const String& apSSID);
    void stopCaptivePortal();

    void loop();   // call every main-loop iteration

    WiFiStatus status() const { return _status; }
    bool       isConnected() const;
    String     localIP() const;

private:
    WiFiStatus _status      = WiFiStatus::DISCONNECTED;
    ConnectCb  _onConnected;
    ConnectCb  _onFailed;
    uint32_t   _connectStart = 0;
    uint32_t   _timeout      = 15000;
    bool       _portalActive = false;
    uint32_t   _lastRecheck  = 0;
};
