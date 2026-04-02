/**
 * ui/PlayerScreen.cpp
 *
 * Complete player UI with:
 *  - Album art with JPEG decode (TJpg_Decoder) + placeholder
 *  - Marquee-scroll track title
 *  - Animated progress bar
 *  - Touch controls with visual feedback
 *  - Volume slider
 *  - WiFi indicator
 */
#include "PlayerScreen.h"
#include "UIManager.h"
#include "../AppConfig.h"
#include "../api/RestClient.h"
#include "../storage/Storage.h"
#include <TJpg_Decoder.h>

// External reference to UI manager for settings button
extern UIManager* g_uiManager;

// ── TJpg_Decoder callback (render JPEG tile into LVGL canvas) ─────────
static lv_obj_t*  s_artCanvas    = nullptr;
static lv_color_t s_artBuf[80 * 80];   // max thumbnail 80×80

static bool tjpgOutputCb(int16_t x, int16_t y, uint16_t w, uint16_t h,
                          uint16_t* bitmap) {
    if (!s_artCanvas) return false;
    // Write pixels into canvas buffer
    lv_canvas_copy_buf(s_artCanvas,
                       (const void*)bitmap, x, y, w, h);
    return true;
}

// ── Style helpers ─────────────────────────────────────────────────────
#define MAKE_COLOR(hex)  lv_color_hex(hex)

// ─────────────────────────────────────────────────────────────────────
void PlayerScreen::_buildStyles() {
    // Card style
    lv_style_init(&_styleCard);
    lv_style_set_bg_color(&_styleCard, MAKE_COLOR(COL_CARD));
    lv_style_set_bg_opa(&_styleCard, LV_OPA_COVER);
    lv_style_set_radius(&_styleCard, 12);
    lv_style_set_border_width(&_styleCard, 0);
    lv_style_set_pad_all(&_styleCard, 8);

    // Transport button style
    lv_style_init(&_styleBtn);
    lv_style_set_bg_color(&_styleBtn, MAKE_COLOR(COL_ACCENT));
    lv_style_set_bg_opa(&_styleBtn, LV_OPA_COVER);
    lv_style_set_radius(&_styleBtn, 50);
    lv_style_set_border_width(&_styleBtn, 0);
    lv_style_set_shadow_width(&_styleBtn, 0);
    lv_style_set_text_color(&_styleBtn, MAKE_COLOR(COL_TEXT_PRI));

    // Play/pause button
    lv_style_init(&_styleBtnPlay);
    lv_style_set_bg_color(&_styleBtnPlay, MAKE_COLOR(COL_PLAY_BTN));
    lv_style_set_bg_opa(&_styleBtnPlay, LV_OPA_COVER);
    lv_style_set_radius(&_styleBtnPlay, 50);
    lv_style_set_border_width(&_styleBtnPlay, 0);
    lv_style_set_shadow_width(&_styleBtnPlay, 10);
    lv_style_set_shadow_color(&_styleBtnPlay, MAKE_COLOR(COL_PLAY_BTN));
    lv_style_set_shadow_opa(&_styleBtnPlay, LV_OPA_40);
    lv_style_set_text_color(&_styleBtnPlay, MAKE_COLOR(0x0D0D0D));
}

// ── create ────────────────────────────────────────────────────────────
void PlayerScreen::create(lv_obj_t* parent,
                          PlayerState* state,
                          RestClient*  rest,
                          Storage*     storage) {
    _state   = state;
    _rest    = rest;
    _storage = storage;

    _buildStyles();

    _scr = lv_obj_create(NULL);
    lv_obj_set_size(_scr, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(_scr, MAKE_COLOR(COL_BG), 0);
    lv_obj_set_style_bg_opa(_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_scr, 0, 0);
    lv_obj_set_scroll_dir(_scr, LV_DIR_NONE);
    lv_obj_clear_flag(_scr, LV_OBJ_FLAG_SCROLLABLE);

    _buildHeader(_scr);
    _buildArtPanel(_scr);
    _buildTrackInfo(_scr);
    _buildProgress(_scr);
    _buildControls(_scr);
    _buildVolumeRow(_scr);

    // Setup TJpg_Decoder
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tjpgOutputCb);
}

// ── Header bar ───────────────────────────────────────────────────────
void PlayerScreen::_buildHeader(lv_obj_t* parent) {
    lv_obj_t* hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, DISP_W, 28);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, MAKE_COLOR(COL_SURFACE), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 4, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    _headerLabel = lv_label_create(hdr);
    lv_label_set_text(_headerLabel, LV_SYMBOL_AUDIO " Music Assistant");
    lv_obj_set_style_text_color(_headerLabel, MAKE_COLOR(COL_TEXT_SEC), 0);
    lv_obj_set_style_text_font(_headerLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(_headerLabel, LV_ALIGN_LEFT_MID, 4, 0);

    // WiFi icon (right side)
    _wifiIcon = lv_label_create(hdr);
    lv_label_set_text(_wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(_wifiIcon, MAKE_COLOR(COL_PLAY_BTN), 0);
    lv_obj_align(_wifiIcon, LV_ALIGN_RIGHT_MID, -28, 0);

    // Settings button
    _settingsBtn = lv_btn_create(hdr);
    lv_obj_set_size(_settingsBtn, 22, 20);
    lv_obj_align(_settingsBtn, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_bg_color(_settingsBtn, MAKE_COLOR(COL_SURFACE), 0);
    lv_obj_set_style_border_width(_settingsBtn, 0, 0);
    lv_obj_set_style_shadow_width(_settingsBtn, 0, 0);
    lv_obj_add_event_cb(_settingsBtn, _onSettings, LV_EVENT_CLICKED, this);
    lv_obj_t* settLbl = lv_label_create(_settingsBtn);
    lv_label_set_text(settLbl, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(settLbl, MAKE_COLOR(COL_TEXT_SEC), 0);
    lv_obj_center(settLbl);
}

// ── Album art panel ───────────────────────────────────────────────────
void PlayerScreen::_buildArtPanel(lv_obj_t* parent) {
    _artPanel = lv_obj_create(parent);
    lv_obj_set_size(_artPanel, 130, 130);
    lv_obj_align(_artPanel, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_add_style(_artPanel, &_styleCard, 0);
    lv_obj_set_style_pad_all(_artPanel, 0, 0);
    lv_obj_set_style_clip_corner(_artPanel, true, 0);
    lv_obj_clear_flag(_artPanel, LV_OBJ_FLAG_SCROLLABLE);

    // Canvas for JPEG rendering (80×80 scaled in the 130×130 container)
    s_artCanvas = lv_canvas_create(_artPanel);
    lv_canvas_set_buffer(s_artCanvas, s_artBuf, 80, 80, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(s_artCanvas, 130, 130);
    lv_obj_align(s_artCanvas, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(s_artCanvas, LV_OBJ_FLAG_HIDDEN);

    // Placeholder (music note icon)
    _artPlaceholder = lv_obj_create(_artPanel);
    lv_obj_set_size(_artPlaceholder, 130, 130);
    lv_obj_align(_artPlaceholder, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(_artPlaceholder, MAKE_COLOR(COL_HIGHLIGHT), 0);
    lv_obj_set_style_bg_opa(_artPlaceholder, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_artPlaceholder, 0, 0);
    lv_obj_clear_flag(_artPlaceholder, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* noteIcon = lv_label_create(_artPlaceholder);
    lv_label_set_text(noteIcon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(noteIcon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(noteIcon, MAKE_COLOR(COL_TEXT_PRI), 0);
    lv_obj_center(noteIcon);
}

// ── Track info ────────────────────────────────────────────────────────
void PlayerScreen::_buildTrackInfo(lv_obj_t* parent) {
    _titleLabel = lv_label_create(parent);
    lv_obj_set_width(_titleLabel, DISP_W - 16);
    lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, 174);
    lv_label_set_long_mode(_titleLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(_titleLabel, "Not Playing");
    lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_titleLabel, MAKE_COLOR(COL_TEXT_PRI), 0);
    lv_obj_set_style_text_align(_titleLabel, LV_TEXT_ALIGN_CENTER, 0);

    _artistLabel = lv_label_create(parent);
    lv_obj_set_width(_artistLabel, DISP_W - 16);
    lv_obj_align(_artistLabel, LV_ALIGN_TOP_MID, 0, 195);
    lv_label_set_long_mode(_artistLabel, LV_LABEL_LONG_DOT);
    lv_label_set_text(_artistLabel, "—");
    lv_obj_set_style_text_font(_artistLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_artistLabel, MAKE_COLOR(COL_TEXT_SEC), 0);
    lv_obj_set_style_text_align(_artistLabel, LV_TEXT_ALIGN_CENTER, 0);
}

// ── Progress bar ──────────────────────────────────────────────────────
void PlayerScreen::_buildProgress(lv_obj_t* parent) {
    _progressBar = lv_bar_create(parent);
    lv_obj_set_size(_progressBar, DISP_W - 32, 4);
    lv_obj_align(_progressBar, LV_ALIGN_TOP_MID, 0, 218);
    lv_bar_set_range(_progressBar, 0, 1000);
    lv_bar_set_value(_progressBar, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(_progressBar, MAKE_COLOR(COL_ACCENT), 0);
    lv_obj_set_style_bg_color(_progressBar, MAKE_COLOR(COL_PLAY_BTN),
                              LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_progressBar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(_progressBar, 2, 0);
    lv_obj_set_style_radius(_progressBar, 2, LV_PART_INDICATOR);

    _timeLabel = lv_label_create(parent);
    lv_obj_align(_timeLabel, LV_ALIGN_TOP_RIGHT, -8, 225);
    lv_label_set_text(_timeLabel, "0:00");
    lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_timeLabel, MAKE_COLOR(COL_TEXT_SEC), 0);
}

// ── Transport controls ────────────────────────────────────────────────
void PlayerScreen::_buildControls(lv_obj_t* parent) {
    const int y = 242;
    const int btnSm = 36;   // prev/next
    const int btnLg = 46;   // play/pause

    // Previous
    _prevBtn = lv_btn_create(parent);
    lv_obj_set_size(_prevBtn, btnSm, btnSm);
    lv_obj_align(_prevBtn, LV_ALIGN_TOP_MID, -68, y);
    lv_obj_add_style(_prevBtn, &_styleBtn, 0);
    lv_obj_add_event_cb(_prevBtn, _onPrev, LV_EVENT_CLICKED, this);
    lv_obj_t* prevLbl = lv_label_create(_prevBtn);
    lv_label_set_text(prevLbl, LV_SYMBOL_PREV);
    lv_obj_center(prevLbl);

    // Play/Pause (centre, larger)
    _playBtn = lv_btn_create(parent);
    lv_obj_set_size(_playBtn, btnLg, btnLg);
    lv_obj_align(_playBtn, LV_ALIGN_TOP_MID, 0, y - 5);
    lv_obj_add_style(_playBtn, &_styleBtnPlay, 0);
    lv_obj_add_event_cb(_playBtn, _onPlayPause, LV_EVENT_CLICKED, this);
    lv_obj_t* playLbl = lv_label_create(_playBtn);
    lv_label_set_text(playLbl, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(playLbl, MAKE_COLOR(0x0D0D0D), 0);
    lv_obj_center(playLbl);

    // Next
    _nextBtn = lv_btn_create(parent);
    lv_obj_set_size(_nextBtn, btnSm, btnSm);
    lv_obj_align(_nextBtn, LV_ALIGN_TOP_MID, 68, y);
    lv_obj_add_style(_nextBtn, &_styleBtn, 0);
    lv_obj_add_event_cb(_nextBtn, _onNext, LV_EVENT_CLICKED, this);
    lv_obj_t* nextLbl = lv_label_create(_nextBtn);
    lv_label_set_text(nextLbl, LV_SYMBOL_NEXT);
    lv_obj_center(nextLbl);
}

// ── Volume slider ─────────────────────────────────────────────────────
void PlayerScreen::_buildVolumeRow(lv_obj_t* parent) {
    const int y = 296;

    _volIcon = lv_label_create(parent);
    lv_label_set_text(_volIcon, LV_SYMBOL_VOLUME_MID);
    lv_obj_align(_volIcon, LV_ALIGN_TOP_LEFT, 8, y);
    lv_obj_set_style_text_color(_volIcon, MAKE_COLOR(COL_TEXT_SEC), 0);

    _volSlider = lv_slider_create(parent);
    lv_obj_set_size(_volSlider, DISP_W - 52, 6);
    lv_obj_align(_volSlider, LV_ALIGN_TOP_LEFT, 32, y + 5);
    lv_slider_set_range(_volSlider, 0, 100);
    lv_slider_set_value(_volSlider, 50, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(_volSlider, MAKE_COLOR(COL_ACCENT), 0);
    lv_obj_set_style_bg_color(_volSlider, MAKE_COLOR(COL_PLAY_BTN),
                              LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_volSlider, MAKE_COLOR(COL_PLAY_BTN),
                              LV_PART_KNOB);
    lv_obj_set_style_radius(_volSlider, 3, LV_PART_KNOB);
    lv_obj_set_style_pad_all(_volSlider, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(_volSlider, _onVolChanged, LV_EVENT_VALUE_CHANGED, this);
}

// ── Partial update methods (called by UIManager) ──────────────────────
void PlayerScreen::updatePlayState(PlayState ps) {
    if (!_playBtn) return;
    lv_obj_t* lbl = lv_obj_get_child(_playBtn, 0);
    if (!lbl) return;
    bool playing = (ps == PlayState::PLAYING);
    lv_label_set_text(lbl, playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

void PlayerScreen::updateTrackInfo(const PlayerSnapshot& snap) {
    if (!_titleLabel) return;

    const String& t = snap.currentItem.title;
    lv_label_set_text(_titleLabel, t.isEmpty() ? "Not Playing" : t.c_str());

    String sub = snap.currentItem.artist;
    if (!snap.currentItem.album.isEmpty()) {
        if (!sub.isEmpty()) sub += "  ·  ";
        sub += snap.currentItem.album;
    }
    lv_label_set_text(_artistLabel, sub.isEmpty() ? "—" : sub.c_str());

    // Player name in header
    if (!snap.playerName.isEmpty())
        lv_label_set_text(_headerLabel,
            (String(LV_SYMBOL_AUDIO " ") + snap.playerName).c_str());
}

void PlayerScreen::updateProgress(float elapsed, float duration) {
    if (!_progressBar || !_timeLabel) return;

    int pct = 0;
    if (duration > 0) pct = (int)((elapsed / duration) * 1000.0f);
    pct = constrain(pct, 0, 1000);
    lv_bar_set_value(_progressBar, pct, LV_ANIM_ON);

    String t = _formatTime(elapsed);
    if (duration > 0) t += " / " + _formatTime(duration);
    lv_label_set_text(_timeLabel, t.c_str());
}

void PlayerScreen::updateVolume(int vol) {
    if (_volSlider) lv_slider_set_value(_volSlider, vol, LV_ANIM_ON);
}

void PlayerScreen::setConnectionStatus(bool connected) {
    if (!_wifiIcon) return;
    lv_obj_set_style_text_color(_wifiIcon,
        lv_color_hex(connected ? COL_PLAY_BTN : 0xFF4444), 0);
}

// ── Album art loading ─────────────────────────────────────────────────
void PlayerScreen::updateAlbumArt(const String& url, RestClient* rest) {
    if (url == _currentArtUrl) return;
    _currentArtUrl = url;

    if (url.isEmpty() || !rest) {
        _showArtPlaceholder("");
        return;
    }
    _loadArtAsync(url);
}

void PlayerScreen::_showArtPlaceholder(const String& title) {
    lv_obj_add_flag(s_artCanvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_artPlaceholder, LV_OBJ_FLAG_HIDDEN);
    _artLoaded = false;
}

void PlayerScreen::_loadArtAsync(const String& url) {
    // Fetch and decode JPEG on the fly (blocking but short — ~200ms for 80px)
    uint8_t* buf = nullptr;
    size_t   len = 0;

    bool ok = _rest->fetchAlbumArt(url, &buf, &len);
    if (!ok || !buf) {
        _showArtPlaceholder("");
        return;
    }

    // Clear canvas first
    lv_canvas_fill_bg(s_artCanvas, MAKE_COLOR(COL_CARD), LV_OPA_COVER);

    JRESULT res = TJpgDec.drawJpg(0, 0, buf, len);
    free(buf);

    if (res == JDR_OK) {
        lv_obj_clear_flag(s_artCanvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_artPlaceholder, LV_OBJ_FLAG_HIDDEN);
        _artLoaded = true;
        LOG_I("UI", "Album art loaded OK");
    } else {
        LOG_W("UI", "JPEG decode error %d", res);
        _showArtPlaceholder("");
    }
}

// ── destroy ───────────────────────────────────────────────────────────
void PlayerScreen::destroy() {
    if (_artBuf) { free(_artBuf); _artBuf = nullptr; }
    if (_scr)    { lv_obj_del(_scr); _scr = nullptr; }
    s_artCanvas = nullptr;
}

// ── tick ─────────────────────────────────────────────────────────────
void PlayerScreen::tick() {}  // reserved for future animations

// ── Event callbacks ───────────────────────────────────────────────────
void PlayerScreen::_onPlayPause(lv_event_t* e) {
    PlayerScreen* self = (PlayerScreen*)lv_event_get_user_data(e);
    if (!self || !self->_rest || !self->_state) return;
    String pid = self->_state->getPlayerId();
    if (pid.isEmpty()) return;
    if (self->_state->isPlaying()) {
        self->_rest->cmdPause(pid);
        self->_state->applyPlayState(PlayState::PAUSED);
    } else {
        self->_rest->cmdPlay(pid);
        self->_state->applyPlayState(PlayState::PLAYING);
    }
}

void PlayerScreen::_onNext(lv_event_t* e) {
    PlayerScreen* self = (PlayerScreen*)lv_event_get_user_data(e);
    if (!self || !self->_rest || !self->_state) return;
    String pid = self->_state->getPlayerId();
    if (!pid.isEmpty()) self->_rest->cmdNext(pid);
}

void PlayerScreen::_onPrev(lv_event_t* e) {
    PlayerScreen* self = (PlayerScreen*)lv_event_get_user_data(e);
    if (!self || !self->_rest || !self->_state) return;
    String pid = self->_state->getPlayerId();
    if (!pid.isEmpty()) self->_rest->cmdPrevious(pid);
}

void PlayerScreen::_onVolChanged(lv_event_t* e) {
    PlayerScreen* self = (PlayerScreen*)lv_event_get_user_data(e);
    if (!self || !self->_rest || !self->_state) return;
    int vol = lv_slider_get_value(self->_volSlider);
    self->_state->applyVolume(vol);
    String pid = self->_state->getPlayerId();
    if (!pid.isEmpty()) self->_rest->cmdVolumeSet(pid, vol);
}

void PlayerScreen::_onSettings(lv_event_t* e) {
    if (g_uiManager) g_uiManager->showSettingsScreen();
}

// ── Helpers ───────────────────────────────────────────────────────────
String PlayerScreen::_formatTime(float secs) {
    int s = (int)secs;
    int m = s / 60; s %= 60;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return String(buf);
}
