#pragma once
/**
 * ui/UIManager.h
 *
 * Owns all LVGL screens, drives state-based partial redraws,
 * and provides the single interface for non-UI code to trigger
 * screen switches or status updates.
 */
#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "../state/PlayerState.h"

// Forward declarations
class Storage;
class RestClient;
class WebSocketClient;
class WiFiManager;
class PlayerScreen;
class SettingsScreen;

enum class ActiveScreen { NONE, LOADING, PLAYER, SETTINGS, PLAYER_SELECT };

class UIManager {
public:
    void begin(Storage*         storage,
               PlayerState*     state,
               RestClient*      rest,
               WebSocketClient* ws,
               WiFiManager*     wifi);

    // ── Screen transitions ─────────────────────────────────────────
    void showLoadingScreen(const String& message = "");
    void showPlayerScreen();
    void showSettingsScreen();
    void showPlayerSelectScreen(const std::vector<PlayerSnapshot>& players);
    void showStatus(const String& msg);   // overlay toast

    // ── Called from main loop ──────────────────────────────────────
    void update();

    // ── Called by network layer ────────────────────────────────────
    void setConnectionIcon(bool connected);
    void onPlayerListReceived(const std::vector<PlayerSnapshot>& players);

    ActiveScreen currentScreen() const { return _active; }

private:
    Storage*         _storage = nullptr;
    PlayerState*     _state   = nullptr;
    RestClient*      _rest    = nullptr;
    WebSocketClient* _ws      = nullptr;
    WiFiManager*     _wifi    = nullptr;

    PlayerScreen*    _playerScreen   = nullptr;
    SettingsScreen*  _settingsScreen = nullptr;

    ActiveScreen     _active    = ActiveScreen::NONE;
    lv_obj_t*        _loadScr   = nullptr;
    lv_obj_t*        _loadLabel = nullptr;
    lv_obj_t*        _spinner   = nullptr;
    lv_obj_t*        _toastLabel= nullptr;
    uint32_t         _toastEnd  = 0;

    uint32_t         _lastUpdate = 0;

    void _buildLoadingScreen();
    void _buildToast(const String& msg, uint32_t durationMs = 2500);
    void _clearToast();
};
