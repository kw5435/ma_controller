#pragma once
/**
 * state/PlayerState.h
 *
 * Single source of truth for everything the UI displays.
 * Thread-safe dirty-flag pattern: setters mark fields dirty,
 * UIManager only redraws changed fields.
 */
#include <Arduino.h>

// ── Playback states ──────────────────────────────────────────────────
enum class PlayState { UNKNOWN, PLAYING, PAUSED, IDLE, OFF };

// ── Raw data from Music Assistant API ───────────────────────────────
struct QueueItem {
    String title;
    String artist;
    String album;
    String imageUrl;
    float  duration;     // seconds
};

// ── Full player snapshot ─────────────────────────────────────────────
struct PlayerSnapshot {
    String     playerId;
    String     playerName;
    PlayState  state       = PlayState::UNKNOWN;
    QueueItem  currentItem;
    float      elapsedTime = 0.0f;
    int        volume      = 50;   // 0-100
    bool       muted       = false;
    bool       shuffled    = false;
    String     repeatMode  = "off";  // off / one / all
    uint32_t   lastUpdated = 0;
};

// ── Dirty flags so UI only redraws changed parts ─────────────────────
struct DirtyFlags {
    bool playState  = true;
    bool trackInfo  = true;
    bool albumArt   = true;
    bool volume     = true;
    bool progress   = true;
    bool shuffle    = true;
    bool repeat     = true;
    bool player     = true;

    void setAll()   { playState=trackInfo=albumArt=volume=
                      progress=shuffle=repeat=player=true; }
    void clearAll() { playState=trackInfo=albumArt=volume=
                      progress=shuffle=repeat=player=false; }
    bool any() const { return playState||trackInfo||albumArt||
                              volume||progress||shuffle||repeat||player; }
};

// ── PlayerState class ────────────────────────────────────────────────
class PlayerState {
public:
    PlayerState();

    // ── Update from WebSocket / REST (call from network thread) ──────
    void applySnapshot(const PlayerSnapshot& snap);
    void applyPlayState(PlayState ps);
    void applyVolume(int vol);
    void applyElapsed(float t);
    void applyTrack(const QueueItem& item);
    void applyMute(bool m);

    // ── Local tick (call every second from main loop) ─────────────
    void tick();          // advances elapsedTime locally between updates

    // ── Read (from UI thread) ────────────────────────────────────────
    PlayerSnapshot getSnapshot() const;
    DirtyFlags     getDirtyAndClear();   // atomically read + clear flags

    // ── Convenience ─────────────────────────────────────────────────
    bool isPlaying()  const;
    int  getVolume()  const;
    float getElapsed()const;
    float getDuration()const;
    String getPlayerId() const;
    bool hasTrack()   const;

private:
    PlayerSnapshot _snap;
    DirtyFlags     _dirty;
    uint32_t       _lastTickMs = 0;

    // Minimal lock using a volatile flag (single-core ESP32 loop is fine
    // but WebSocket callback runs in same task, so this suffices)
    volatile bool  _lock = false;
    void acquire();
    void release();
};
