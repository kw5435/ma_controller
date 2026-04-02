/**
 * ui/SettingsScreen.cpp
 */
#include "SettingsScreen.h"
#include "UIManager.h"
#include "../AppConfig.h"
#include "../api/RestClient.h"
#include "../display/DisplaySetup.h"

#define MAKE_COLOR(hex) lv_color_hex(hex)
#define FIELD_H   36
#define FIELD_GAP  6

// ── create ────────────────────────────────────────────────────────────
void SettingsScreen::create(lv_obj_t* parent,
                            UIManager* uiMgr,
                            Storage*   storage,
                            RestClient* rest) {
    _ui      = uiMgr;
    _storage = storage;
    _rest    = rest;

    AppSettings s = storage->loadSettings();

    _scr = lv_obj_create(NULL);
    lv_obj_set_size(_scr, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(_scr, MAKE_COLOR(COL_BG), 0);
    lv_obj_set_style_bg_opa(_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_scr, 0, 0);
    lv_obj_set_scroll_dir(_scr, LV_DIR_VER);

    // ── Header ────────────────────────────────────────────────────
    lv_obj_t* hdr = lv_obj_create(_scr);
    lv_obj_set_size(hdr, DISP_W, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, MAKE_COLOR(COL_SURFACE), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* backBtn = lv_btn_create(hdr);
    lv_obj_set_size(backBtn, 48, 24);
    lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_color(backBtn, MAKE_COLOR(COL_SURFACE), 0);
    lv_obj_set_style_border_width(backBtn, 0, 0);
    lv_obj_set_style_shadow_width(backBtn, 0, 0);
    lv_obj_add_event_cb(backBtn, _onBack, LV_EVENT_CLICKED, this);
    lv_obj_t* backLbl = lv_label_create(backBtn);
    lv_label_set_text(backLbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(backLbl, MAKE_COLOR(COL_PLAY_BTN), 0);
    lv_obj_set_style_text_font(backLbl, &lv_font_montserrat_12, 0);
    lv_obj_center(backLbl);

    lv_obj_t* titleLbl = lv_label_create(hdr);
    lv_label_set_text(titleLbl, "Settings");
    lv_obj_set_style_text_font(titleLbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(titleLbl, MAKE_COLOR(COL_TEXT_PRI), 0);
    lv_obj_align(titleLbl, LV_ALIGN_CENTER, 0, 0);

    // ── Scrollable content container ──────────────────────────────
    lv_obj_t* cont = lv_obj_create(_scr);
    lv_obj_set_size(cont, DISP_W, DISP_H - 30);
    lv_obj_set_pos(cont, 0, 30);
    lv_obj_set_style_bg_color(cont, MAKE_COLOR(COL_BG), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 8, 0);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);

    int y = 0;

    // ── Section: Network ─────────────────────────────────────────
    lv_obj_t* netLabel = lv_label_create(cont);
    lv_label_set_text(netLabel, "NETWORK");
    lv_obj_set_pos(netLabel, 0, y);
    lv_obj_set_style_text_font(netLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(netLabel, MAKE_COLOR(COL_HIGHLIGHT), 0);
    y += 18;

    _taSSID = _makeField(cont, "WiFi SSID", s.ssid, false, y);        y += FIELD_H + FIELD_GAP;
    _taPass = _makeField(cont, "WiFi Password", s.password, true, y); y += FIELD_H + FIELD_GAP;

    // ── Section: Music Assistant ──────────────────────────────────
    y += 4;
    lv_obj_t* maLabel = lv_label_create(cont);
    lv_label_set_text(maLabel, "MUSIC ASSISTANT");
    lv_obj_set_pos(maLabel, 0, y);
    lv_obj_set_style_text_font(maLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(maLabel, MAKE_COLOR(COL_HIGHLIGHT), 0);
    y += 18;

    _taIP    = _makeField(cont, "Server IP",  s.serverIP,              false, y); y += FIELD_H + FIELD_GAP;
    _taPort  = _makeField(cont, "Port",       String(s.serverPort),    false, y); y += FIELD_H + FIELD_GAP;
    _taToken = _makeField(cont, "API Token",  s.apiToken,              true,  y); y += FIELD_H + FIELD_GAP;

    // ── Section: Display ─────────────────────────────────────────
    y += 4;
    lv_obj_t* dispLabel = lv_label_create(cont);
    lv_label_set_text(dispLabel, "DISPLAY");
    lv_obj_set_pos(dispLabel, 0, y);
    lv_obj_set_style_text_font(dispLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(dispLabel, MAKE_COLOR(COL_HIGHLIGHT), 0);
    y += 18;

    lv_obj_t* brightLabel = lv_label_create(cont);
    lv_label_set_text(brightLabel, "Brightness");
    lv_obj_set_pos(brightLabel, 0, y);
    lv_obj_set_style_text_font(brightLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(brightLabel, MAKE_COLOR(COL_TEXT_SEC), 0);
    y += 14;

    _sldBright = lv_slider_create(cont);
    lv_obj_set_size(_sldBright, DISP_W - 20, 6);
    lv_obj_set_pos(_sldBright, 0, y + 4);
    lv_slider_set_range(_sldBright, 20, 255);
    lv_slider_set_value(_sldBright, s.brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(_sldBright, MAKE_COLOR(COL_ACCENT), 0);
    lv_obj_set_style_bg_color(_sldBright, MAKE_COLOR(COL_PLAY_BTN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_sldBright, MAKE_COLOR(COL_PLAY_BTN), LV_PART_KNOB);
    lv_obj_add_event_cb(_sldBright, _onBrightChanged, LV_EVENT_VALUE_CHANGED, this);
    y += FIELD_H;

    // ── Player list label ────────────────────────────────────────
    y += 4;
    lv_obj_t* playerLabel = lv_label_create(cont);
    lv_label_set_text(playerLabel, "SELECT PLAYER");
    lv_obj_set_pos(playerLabel, 0, y);
    lv_obj_set_style_text_font(playerLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(playerLabel, MAKE_COLOR(COL_HIGHLIGHT), 0);
    y += 18;

    _playerList = lv_list_create(cont);
    lv_obj_set_size(_playerList, DISP_W - 20, 80);
    lv_obj_set_pos(_playerList, 0, y);
    lv_obj_set_style_bg_color(_playerList, MAKE_COLOR(COL_CARD), 0);
    lv_obj_set_style_border_color(_playerList, MAKE_COLOR(COL_ACCENT), 0);
    lv_obj_set_style_border_width(_playerList, 1, 0);
    lv_obj_set_style_radius(_playerList, 8, 0);
    lv_list_add_text(_playerList, "Fetching players...");
    y += 90;

    // ── Status label ─────────────────────────────────────────────
    _statusLabel = lv_label_create(cont);
    lv_label_set_text(_statusLabel, "");
    lv_obj_set_pos(_statusLabel, 0, y);
    lv_obj_set_style_text_font(_statusLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_statusLabel, MAKE_COLOR(COL_PLAY_BTN), 0);
    lv_obj_set_width(_statusLabel, DISP_W - 20);
    y += 20;

    // ── Buttons ──────────────────────────────────────────────────
    _saveBtn = lv_btn_create(cont);
    lv_obj_set_size(_saveBtn, DISP_W - 20, 36);
    lv_obj_set_pos(_saveBtn, 0, y);
    lv_obj_set_style_bg_color(_saveBtn, MAKE_COLOR(COL_PLAY_BTN), 0);
    lv_obj_set_style_radius(_saveBtn, 8, 0);
    lv_obj_add_event_cb(_saveBtn, _onSave, LV_EVENT_CLICKED, this);
    lv_obj_t* saveLbl = lv_label_create(_saveBtn);
    lv_label_set_text(saveLbl, LV_SYMBOL_SAVE "  Save & Connect");
    lv_obj_set_style_text_color(saveLbl, MAKE_COLOR(0x0D0D0D), 0);
    lv_obj_set_style_text_font(saveLbl, &lv_font_montserrat_14, 0);
    lv_obj_center(saveLbl);
    y += 44;

    _resetBtn = lv_btn_create(cont);
    lv_obj_set_size(_resetBtn, DISP_W - 20, 32);
    lv_obj_set_pos(_resetBtn, 0, y);
    lv_obj_set_style_bg_color(_resetBtn, MAKE_COLOR(0x3A0000), 0);
    lv_obj_set_style_radius(_resetBtn, 8, 0);
    lv_obj_add_event_cb(_resetBtn, _onReset, LV_EVENT_CLICKED, this);
    lv_obj_t* resetLbl = lv_label_create(_resetBtn);
    lv_label_set_text(resetLbl, LV_SYMBOL_WARNING "  Factory Reset");
    lv_obj_set_style_text_color(resetLbl, MAKE_COLOR(0xFF6666), 0);
    lv_obj_set_style_text_font(resetLbl, &lv_font_montserrat_12, 0);
    lv_obj_center(resetLbl);
    y += 44;

    // ── Shared keyboard (hidden until a field is tapped) ─────────
    _keyboard = lv_keyboard_create(_scr);
    lv_obj_set_size(_keyboard, DISP_W, 140);
    lv_obj_align(_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(_keyboard, MAKE_COLOR(COL_SURFACE), 0);

    // Attach focus handlers to all text areas
    for (lv_obj_t* ta : {_taSSID, _taPass, _taIP, _taPort, _taToken})
        lv_obj_add_event_cb(ta, _onTAFocused, LV_EVENT_FOCUSED, this);

    // Auto-fetch player list if server is configured
    if (!s.serverIP.isEmpty() && rest) {
        rest->fetchPlayers(
            [this](std::vector<PlayerSnapshot> players) {
                showPlayerList(players);
            }, nullptr);
    }
}

// ── Field builder ─────────────────────────────────────────────────────
lv_obj_t* SettingsScreen::_makeField(lv_obj_t* parent, const char* label,
                                     const String& value, bool isPassword,
                                     int y) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_pos(lbl, 0, y);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, MAKE_COLOR(COL_TEXT_SEC), 0);

    lv_obj_t* ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, DISP_W - 20, FIELD_H - 6);
    lv_obj_set_pos(ta, 0, y + 14);
    lv_textarea_set_text(ta, value.c_str());
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_password_mode(ta, isPassword);
    lv_obj_set_style_bg_color(ta, MAKE_COLOR(COL_CARD), 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ta, MAKE_COLOR(COL_ACCENT), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 6, 0);
    lv_obj_set_style_text_color(ta, MAKE_COLOR(COL_TEXT_PRI), 0);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ta, MAKE_COLOR(COL_PLAY_BTN), LV_PART_CURSOR);
    return ta;
}

// ── Player list rendering ─────────────────────────────────────────────
void SettingsScreen::showPlayerList(const std::vector<PlayerSnapshot>& players) {
    if (!_playerList) return;
    lv_obj_clean(_playerList);

    if (players.empty()) {
        lv_list_add_text(_playerList, "No players found");
        return;
    }
    for (const auto& p : players) {
        String display = p.playerName.isEmpty() ? p.playerId : p.playerName;
        lv_obj_t* btn = lv_list_add_btn(_playerList,
            LV_SYMBOL_AUDIO, display.c_str());
        lv_obj_set_style_bg_color(btn, MAKE_COLOR(COL_CARD), 0);
        lv_obj_set_style_text_color(btn, MAKE_COLOR(COL_TEXT_PRI), 0);
        // Store player ID in the button's user data
        lv_obj_set_user_data(btn, (void*)p.playerId.c_str());
        lv_obj_add_event_cb(btn, _onPlayerSelected, LV_EVENT_CLICKED, this);
    }
}

void SettingsScreen::destroy() {
    if (_scr) { lv_obj_del(_scr); _scr = nullptr; }
}

// ── Event callbacks ───────────────────────────────────────────────────
void SettingsScreen::_onTAFocused(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (!self || !self->_keyboard) return;
    lv_keyboard_set_textarea(self->_keyboard, lv_event_get_target(e));
    lv_obj_clear_flag(self->_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::_onBrightChanged(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (!self || !self->_sldBright || !self->_storage) return;
    uint8_t v = (uint8_t)lv_slider_get_value(self->_sldBright);
    DisplaySetup::setBrightness(v);
    self->_storage->saveBrightness(v);
}

void SettingsScreen::_onSave(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (!self || !self->_storage) return;

    AppSettings s;
    s.ssid       = lv_textarea_get_text(self->_taSSID);
    s.password   = lv_textarea_get_text(self->_taPass);
    s.serverIP   = lv_textarea_get_text(self->_taIP);
    s.serverPort = (uint16_t)String(lv_textarea_get_text(self->_taPort)).toInt();
    if (s.serverPort == 0) s.serverPort = MA_DEFAULT_PORT;
    s.apiToken   = lv_textarea_get_text(self->_taToken);
    s.brightness = (uint8_t)lv_slider_get_value(self->_sldBright);
    s.configured = !s.ssid.isEmpty() && !s.serverIP.isEmpty();

    self->_storage->saveSettings(s);

    if (self->_statusLabel)
        lv_label_set_text(self->_statusLabel, "Saved! Restarting...");

    LOG_I("SETTINGS", "Saved. Restarting in 1.5s...");
    delay(1500);
    ESP.restart();
}

void SettingsScreen::_onReset(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (!self || !self->_storage) return;

    // Show confirmation msgbox
    static const char* btns[] = {"Yes, Reset", "Cancel", ""};
    lv_obj_t* mb = lv_msgbox_create(lv_scr_act(), "Factory Reset",
        "All settings will be erased. Continue?", btns, false);
    lv_obj_set_style_bg_color(mb, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_text_color(mb, lv_color_hex(COL_TEXT_PRI), 0);
    lv_obj_center(mb);

    lv_obj_add_event_cb(mb, [](lv_event_t* ev) {
        lv_obj_t* box  = lv_event_get_current_target(ev);
        uint16_t  btn  = lv_msgbox_get_active_btn(box);
        if (btn == 0) {  // "Yes, Reset"
            // Grab storage from outer scope via user_data chain
            // Simplest: store pointer in msgbox user_data
            Storage* st = (Storage*)lv_obj_get_user_data(box);
            if (st) st->clearAll();
            delay(500);
            ESP.restart();
        }
        lv_msgbox_close(box);
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    // Attach storage pointer to msgbox for the callback
    lv_obj_set_user_data(mb, self->_storage);
}

void SettingsScreen::_onBack(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (self && self->_ui) self->_ui->showPlayerScreen();
}

void SettingsScreen::_onPlayerSelected(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    const char* pid = (const char*)lv_obj_get_user_data(btn);

    if (self && self->_storage && pid) {
        self->_storage->savePlayerId(String(pid));
        if (self->_statusLabel) {
            String msg = "Player selected: " + String(pid);
            lv_label_set_text(self->_statusLabel, msg.c_str());
        }
        LOG_I("SETTINGS", "Player selected: %s", pid);
    }
}
