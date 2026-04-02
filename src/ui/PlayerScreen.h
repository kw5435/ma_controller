#pragma once
/**
 * ui/PlayerScreen.h
 *
 * Full-screen player UI (240×320 portrait):
 *
 *  ┌──────────────────────────────┐
 *  │  ≡  Player Name         ⚙  📶│  ← header bar (28px)
 *  ├──────────────────────────────┤
 *  │                              │
 *  │   ┌──────────────────────┐   │
 *  │   │   Album Art / Icon   │   │  ← art panel (140×140 centred)
 *  │   └──────────────────────┘   │
 *  │                              │
 *  │   Track Title (bold)         │  ← marquee scroll if long
 *  │   Artist · Album             │
 *  │                              │
 *  │   ██████████░░░░░░░  2:15    │  ← progress bar + time
 *  │                              │
 *  │   ⏮    ⏪    ⏯    ⏩    ⏭   │  ← control row
 *  │                              │
 *  │   🔇 ─────────●──────── 🔊  │  ← volume slider
 *  └──────────────────────────────┘
 */
#include <Arduino.h>
#include <lvgl.h>
#include "../state/PlayerState.h"

class RestClient;
class Storage;

class PlayerScreen {
public:
    PlayerScreen() {}

    void create(lv_obj_t* parent,
                PlayerState* state,
                RestClient*  rest,
                Storage*     storage);

    void destroy();

    // Called by UIManager on dirty flags
    void updatePlayState(PlayState ps);
    void updateTrackInfo(const PlayerSnapshot& snap);
    void updateProgress(float elapsed, float duration);
    void updateVolume(int vol);
    void updateAlbumArt(const String& url, RestClient* rest);
    void setConnectionStatus(bool connected);
    void tick();   // called every loop for progress animation

    lv_obj_t* getScreen() const { return _scr; }

private:
    lv_obj_t* _scr          = nullptr;
    lv_obj_t* _headerLabel  = nullptr;
    lv_obj_t* _wifiIcon     = nullptr;
    lv_obj_t* _artPanel     = nullptr;
    lv_obj_t* _artImg       = nullptr;      // lv_img or placeholder
    lv_obj_t* _artPlaceholder = nullptr;
    lv_obj_t* _titleLabel   = nullptr;
    lv_obj_t* _artistLabel  = nullptr;
    lv_obj_t* _albumLabel   = nullptr;
    lv_obj_t* _progressBar  = nullptr;
    lv_obj_t* _timeLabel    = nullptr;
    lv_obj_t* _playBtn      = nullptr;
    lv_obj_t* _prevBtn      = nullptr;
    lv_obj_t* _nextBtn      = nullptr;
    lv_obj_t* _volSlider    = nullptr;
    lv_obj_t* _volIcon      = nullptr;
    lv_obj_t* _settingsBtn  = nullptr;

    PlayerState* _state  = nullptr;
    RestClient*  _rest   = nullptr;
    Storage*     _storage= nullptr;

    String    _currentArtUrl;
    uint8_t*  _artBuf    = nullptr;
    bool      _artLoaded = false;

    lv_style_t _styleCard;
    lv_style_t _styleBtn;
    lv_style_t _styleBtnPlay;

    void _buildStyles();
    void _buildHeader(lv_obj_t* parent);
    void _buildArtPanel(lv_obj_t* parent);
    void _buildTrackInfo(lv_obj_t* parent);
    void _buildProgress(lv_obj_t* parent);
    void _buildControls(lv_obj_t* parent);
    void _buildVolumeRow(lv_obj_t* parent);

    static void _onPlayPause(lv_event_t* e);
    static void _onNext(lv_event_t* e);
    static void _onPrev(lv_event_t* e);
    static void _onVolChanged(lv_event_t* e);
    static void _onSettings(lv_event_t* e);

    static String _formatTime(float secs);
    void _showArtPlaceholder(const String& title);
    void _loadArtAsync(const String& url);
};
