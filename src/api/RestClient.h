#pragma once
/**
 * api/RestClient.h
 *
 * Music Assistant REST API (v2.x assumed)
 *
 * Base URL : http://<ip>:<port>/api
 * Auth     : Header  x-api-key: <token>  (optional)
 *
 * Endpoints used:
 *   GET  /api/players                          → list all players
 *   GET  /api/players/{id}                     → single player state
 *   POST /api/players/{id}/cmd/play
 *   POST /api/players/{id}/cmd/pause
 *   POST /api/players/{id}/cmd/next
 *   POST /api/players/{id}/cmd/previous
 *   POST /api/players/{id}/cmd/volume_set?volume_level=50
 *   GET  /api/players/{id}/queue               → queue with current track
 *
 * All responses are JSON. See docs/api_schema.md for examples.
 */
#include <Arduino.h>
#include <functional>
#include <vector>
#include <ArduinoJson.h>
#include "../state/PlayerState.h"

class RestClient {
public:
    using PlayerListCb   = std::function<void(std::vector<PlayerSnapshot>)>;
    using PlayerUpdateCb = std::function<void(PlayerSnapshot)>;
    using ErrorCb        = std::function<void(String)>;

    void begin(const String& serverIP, uint16_t port, const String& token);

    // ── Queries ────────────────────────────────────────────────────
    bool fetchPlayers(PlayerListCb onDone, ErrorCb onErr = nullptr);
    bool fetchPlayerState(const String& playerId,
                          PlayerUpdateCb onDone, ErrorCb onErr = nullptr);

    // ── Commands (fire-and-forget, returns HTTP status) ────────────
    int cmdPlay(const String& playerId);
    int cmdPause(const String& playerId);
    int cmdNext(const String& playerId);
    int cmdPrevious(const String& playerId);
    int cmdVolumeSet(const String& playerId, int volumePct);
    int cmdStop(const String& playerId);

    // ── Album art ─────────────────────────────────────────────────
    // Returns raw JPEG bytes; caller owns buffer
    bool fetchAlbumArt(const String& imageUrl,
                       uint8_t** outBuf, size_t* outLen);

    String serverBase() const { return _base; }

private:
    String   _base;
    String   _token;
    uint32_t _timeout = 5000;

    String   _makeUrl(const String& path) const;
    int      _post(const String& url, const String& body = "");
    String   _get(const String& url);
    void     _addAuth(class HTTPClient& http) const;

    PlayerSnapshot _parsePlayer(const JsonObject& obj);
    QueueItem      _parseQueueItem(const JsonObject& obj);
};
