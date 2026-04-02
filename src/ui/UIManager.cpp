/**
 * ui/UIManager.cpp
 */
#include "UIManager.h"
#include "PlayerScreen.h"
#include "SettingsScreen.h"
#include "../AppConfig.h"
#include "../storage/Storage.h"
#include "../api/RestClient.h"
#include "../api/WebSocketClient.h"
#include "../network/WiFiManager.h"

// Global pointer used by screen callbacks that need UIManager
UIManager* g_uiManager = nullptr;

#define MAKE_COLOR(hex) lv_color_hex(hex)

// ── begin ─────────────────────────────────────────────────────────────
void UIManager::begin(Storage*         storage,
                      PlayerState*     state,
                      RestClient*      rest,
                      WebSocketClient* ws,
                      WiFiManager*     wifi) {
    _storage = storage;
    _state   = state;
    _rest    = rest;
    _ws      = ws;
    _wifi    = wifi;

    g_uiManager = this;

    _playerScreen  = new PlayerScreen();
    _settingsScreen = new SettingsScreen();
}

// ── update (main loop) ────────────────────────────────────────────────
void UIManager::update() {
    uint32_t now = millis();

    // Dismiss toast
    if (_toastLabel && _toastEnd > 0 && now > _toastEnd) _clearToast();

    // Throttle UI updates
    if (now - _lastUpdate < UI_UPDATE_INTERVAL_MS) return;
    _lastUpdate = now;

    if (_active != ActiveScreen::PLAYER) return;
    if (!_playerScreen) return;

    DirtyFlags dirty = _state->getDirtyAndClear();
    if (!dirty.any()) return;

    PlayerSnapshot snap = _state->getSnapshot();

    if (dirty.playState)
        _playerScreen->updatePlayState(snap.state);

    if (dirty.trackInfo)
        _playerScreen->updateTrackInfo(snap);

    if (dirty.albumArt)
        _playerScreen->updateAlbumArt(snap.currentItem.imageUrl, _rest);

    if (dirty.volume)
        _playerScreen->updateVolume(snap.volume);

    if (dirty.progress)
        _playerScreen->updateProgress(snap.elapsedTime, snap.currentItem.duration);
}

// ── showLoadingScreen ─────────────────────────────────────────────────
void UIManager::showLoadingScreen(const String& message) {
    lv_obj_t* oldScr = _loadScr;  // keep reference, delete AFTER loading new screen

    _loadScr = lv_obj_create(NULL);
    lv_obj_set_size(_loadScr, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(_loadScr, MAKE_COLOR(COL_BG), 0);
    lv_obj_set_style_bg_opa(_loadScr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_loadScr, LV_OBJ_FLAG_SCROLLABLE);

    // Logo / title
    lv_obj_t* title = lv_label_create(_loadScr);
    lv_label_set_text(title, LV_SYMBOL_AUDIO "  Music Assistant");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, MAKE_COLOR(COL_PLAY_BTN), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -50);

    // Spinner
    _spinner = lv_spinner_create(_loadScr, 1000, 60);
    lv_obj_set_size(_spinner, 48, 48);
    lv_obj_align(_spinner, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_arc_color(_spinner, MAKE_COLOR(COL_PLAY_BTN), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(_spinner, MAKE_COLOR(COL_ACCENT), 0);

    // Status message
    _loadLabel = lv_label_create(_loadScr);
    lv_label_set_text(_loadLabel, message.isEmpty() ? "Connecting..." : message.c_str());
    lv_obj_set_style_text_font(_loadLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_loadLabel, MAKE_COLOR(COL_TEXT_SEC), 0);
    lv_obj_align(_loadLabel, LV_ALIGN_CENTER, 0, 50);

    lv_scr_load(_loadScr);          // make new screen active first
    _active = ActiveScreen::LOADING;

    if (oldScr) {                   // now safe to delete (no longer active)
        _toastLabel = nullptr;
        lv_obj_del(oldScr);
    }
}

void UIManager::showStatus(const String& msg) {
    if (_loadLabel) lv_label_set_text(_loadLabel, msg.c_str());
    LOG_I("UI", "Status: %s", msg.c_str());
}

// ── showPlayerScreen ──────────────────────────────────────────────────
void UIManager::showPlayerScreen() {
    if (_active == ActiveScreen::PLAYER) return;

    if (!_playerScreen) _playerScreen = new PlayerScreen();

    if (_playerScreen->getScreen() == nullptr) {
        _playerScreen->create(NULL, _state, _rest, _storage);
    }

    lv_scr_load_anim(_playerScreen->getScreen(),
                     LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, false);
    _active = ActiveScreen::PLAYER;

    // Force full redraw on first show
    _state->getDirtyAndClear();  // consume any stale flags
    DirtyFlags all; all.setAll();
    // Re-inject flags by applying current snapshot
    PlayerSnapshot snap = _state->getSnapshot();
    _playerScreen->updatePlayState(snap.state);
    _playerScreen->updateTrackInfo(snap);
    _playerScreen->updateVolume(snap.volume);
    _playerScreen->updateProgress(snap.elapsedTime, snap.currentItem.duration);
    if (!snap.currentItem.imageUrl.isEmpty())
        _playerScreen->updateAlbumArt(snap.currentItem.imageUrl, _rest);

    LOG_I("UI", "Player screen loaded");
}

// ── showSettingsScreen ────────────────────────────────────────────────
void UIManager::showSettingsScreen() {
    if (!_settingsScreen) _settingsScreen = new SettingsScreen();

    if (_settingsScreen->getScreen() == nullptr) {
        _settingsScreen->create(NULL, this, _storage, _rest);
    }

    lv_scr_load_anim(_settingsScreen->getScreen(),
                     LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
    _active = ActiveScreen::SETTINGS;
    LOG_I("UI", "Settings screen loaded");
}

// ── showPlayerSelectScreen ────────────────────────────────────────────
void UIManager::showPlayerSelectScreen(const std::vector<PlayerSnapshot>& players) {
    if (_settingsScreen && _settingsScreen->getScreen())
        _settingsScreen->showPlayerList(players);
}

// ── setConnectionIcon ─────────────────────────────────────────────────
void UIManager::setConnectionIcon(bool connected) {
    if (_playerScreen) _playerScreen->setConnectionStatus(connected);
}

void UIManager::onPlayerListReceived(const std::vector<PlayerSnapshot>& players) {
    if (_settingsScreen && _active == ActiveScreen::SETTINGS)
        _settingsScreen->showPlayerList(players);
}

// ── Toast overlay ─────────────────────────────────────────────────────
void UIManager::_buildToast(const String& msg, uint32_t durationMs) {
    _clearToast();

    lv_obj_t* base = lv_scr_act();
    _toastLabel = lv_label_create(base);
    lv_label_set_text(_toastLabel, msg.c_str());
    lv_obj_set_style_bg_color(_toastLabel, MAKE_COLOR(COL_HIGHLIGHT), 0);
    lv_obj_set_style_bg_opa(_toastLabel, LV_OPA_90, 0);
    lv_obj_set_style_text_color(_toastLabel, MAKE_COLOR(COL_TEXT_PRI), 0);
    lv_obj_set_style_text_font(_toastLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_pad_all(_toastLabel, 8, 0);
    lv_obj_set_style_radius(_toastLabel, 8, 0);
    lv_obj_align(_toastLabel, LV_ALIGN_BOTTOM_MID, 0, -12);
    _toastEnd = millis() + durationMs;
}

void UIManager::_clearToast() {
    if (_toastLabel) {
        lv_obj_del(_toastLabel);
        _toastLabel = nullptr;
    }
    _toastEnd = 0;
}
