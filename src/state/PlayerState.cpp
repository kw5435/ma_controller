/**
 * state/PlayerState.cpp
 */
#include "PlayerState.h"
#include "../AppConfig.h"

PlayerState::PlayerState() {
    _snap.state = PlayState::UNKNOWN;
}

// ── Spin-lock helpers (single-threaded Arduino loop + WS callbacks) ──
void PlayerState::acquire() {
    while (_lock) yield();
    _lock = true;
}
void PlayerState::release() { _lock = false; }

// ── Full snapshot (from initial REST fetch or reconnect) ─────────────
void PlayerState::applySnapshot(const PlayerSnapshot& snap) {
    acquire();
    // Detect what changed before overwriting
    bool stateChanged  = (_snap.state != snap.state);
    bool trackChanged  = (_snap.currentItem.title != snap.currentItem.title ||
                          _snap.currentItem.artist != snap.currentItem.artist);
    bool artChanged    = (_snap.currentItem.imageUrl != snap.currentItem.imageUrl);
    bool volChanged    = (_snap.volume != snap.volume);
    bool muteChanged   = (_snap.muted  != snap.muted);
    bool playerChanged = (_snap.playerId != snap.playerId);

    _snap = snap;
    _snap.lastUpdated = millis();

    if (stateChanged)  _dirty.playState = true;
    if (trackChanged)  _dirty.trackInfo = true;
    if (artChanged)    _dirty.albumArt  = true;
    if (volChanged || muteChanged) _dirty.volume = true;
    if (playerChanged) _dirty.player   = true;
    _dirty.progress = true;

    release();
    LOG_I("STATE", "Snapshot: %s | vol=%d | state=%d",
          snap.currentItem.title.c_str(), snap.volume, (int)snap.state);
}

void PlayerState::applyPlayState(PlayState ps) {
    acquire();
    if (_snap.state != ps) { _snap.state = ps; _dirty.playState = true; }
    release();
}

void PlayerState::applyVolume(int vol) {
    acquire();
    vol = constrain(vol, 0, 100);
    if (_snap.volume != vol) { _snap.volume = vol; _dirty.volume = true; }
    release();
}

void PlayerState::applyElapsed(float t) {
    acquire();
    _snap.elapsedTime = t;
    _snap.lastUpdated = millis();
    _dirty.progress = true;
    release();
}

void PlayerState::applyTrack(const QueueItem& item) {
    acquire();
    bool artChanged  = (_snap.currentItem.imageUrl != item.imageUrl);
    bool infoChanged = (_snap.currentItem.title != item.title ||
                        _snap.currentItem.artist != item.artist);
    _snap.currentItem = item;
    if (infoChanged) _dirty.trackInfo = true;
    if (artChanged)  _dirty.albumArt  = true;
    release();
}

void PlayerState::applyMute(bool m) {
    acquire();
    if (_snap.muted != m) { _snap.muted = m; _dirty.volume = true; }
    release();
}

// ── Local tick: advance elapsed time between server updates ──────────
void PlayerState::tick() {
    uint32_t now = millis();
    if (_lastTickMs == 0) { _lastTickMs = now; return; }

    acquire();
    if (_snap.state == PlayState::PLAYING) {
        float dt = (now - _lastTickMs) / 1000.0f;
        _snap.elapsedTime += dt;
        // clamp to duration
        if (_snap.currentItem.duration > 0 &&
            _snap.elapsedTime > _snap.currentItem.duration) {
            _snap.elapsedTime = _snap.currentItem.duration;
        }
        _dirty.progress = true;
    }
    release();
    _lastTickMs = now;
}

// ── Atomic read + clear dirty flags ─────────────────────────────────
DirtyFlags PlayerState::getDirtyAndClear() {
    acquire();
    DirtyFlags f = _dirty;
    _dirty.clearAll();
    release();
    return f;
}

PlayerSnapshot PlayerState::getSnapshot() const {
    return _snap;   // struct copy, safe enough for single-core
}

// ── Convenience ─────────────────────────────────────────────────────
bool   PlayerState::isPlaying()   const { return _snap.state == PlayState::PLAYING; }
int    PlayerState::getVolume()   const { return _snap.volume; }
float  PlayerState::getElapsed()  const { return _snap.elapsedTime; }
float  PlayerState::getDuration() const { return _snap.currentItem.duration; }
String PlayerState::getPlayerId() const { return _snap.playerId; }
bool   PlayerState::hasTrack()    const { return !_snap.currentItem.title.isEmpty(); }
