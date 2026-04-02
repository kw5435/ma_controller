#pragma once
/**
 * ui/SettingsScreen.h
 *
 * Settings UI:
 *  ┌──────────────────────────────┐
 *  │  ← Settings                  │  ← back button + title
 *  ├──────────────────────────────┤
 *  │  WiFi SSID      [_________]  │
 *  │  WiFi Password  [_________]  │
 *  │  MA Server IP   [_________]  │
 *  │  MA Port        [_________]  │
 *  │  API Token      [_________]  │
 *  │  Brightness     [====●====]  │
 *  ├──────────────────────────────┤
 *  │       [ Save & Connect ]     │
 *  │       [ Factory Reset  ]     │
 *  └──────────────────────────────┘
 *  + Player selection list
 */
#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "../state/PlayerState.h"
#include "../storage/Storage.h"

class UIManager;
class RestClient;

class SettingsScreen {
public:
    void create(lv_obj_t* parent,
                UIManager* uiMgr,
                Storage*   storage,
                RestClient* rest);
    void destroy();
    void showPlayerList(const std::vector<PlayerSnapshot>& players);

    lv_obj_t* getScreen() const { return _scr; }

private:
    lv_obj_t* _scr         = nullptr;
    lv_obj_t* _taSSID      = nullptr;
    lv_obj_t* _taPass      = nullptr;
    lv_obj_t* _taIP        = nullptr;
    lv_obj_t* _taPort      = nullptr;
    lv_obj_t* _taToken     = nullptr;
    lv_obj_t* _sldBright   = nullptr;
    lv_obj_t* _keyboard    = nullptr;
    lv_obj_t* _saveBtn     = nullptr;
    lv_obj_t* _resetBtn    = nullptr;
    lv_obj_t* _playerList  = nullptr;
    lv_obj_t* _statusLabel = nullptr;

    UIManager* _ui      = nullptr;
    Storage*   _storage = nullptr;
    RestClient* _rest   = nullptr;

    lv_obj_t* _makeField(lv_obj_t* parent, const char* label,
                         const String& value, bool isPassword,
                         int y);
    void _attachKeyboard(lv_obj_t* ta);

    static void _onSave(lv_event_t* e);
    static void _onReset(lv_event_t* e);
    static void _onBack(lv_event_t* e);
    static void _onTAFocused(lv_event_t* e);
    static void _onBrightChanged(lv_event_t* e);
    static void _onPlayerSelected(lv_event_t* e);
};
