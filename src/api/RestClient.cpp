/**
 * api/RestClient.cpp
 *
 * Music Assistant API JSON shapes (documented assumptions):
 *
 * GET /api/players → array of player objects:
 * {
 *   "player_id": "spotify_player_abc",
 *   "display_name": "Living Room",
 *   "powered": true,
 *   "state": "playing",       // playing | paused | idle | off
 *   "volume_level": 72,       // 0-100
 *   "muted": false,
 *   "current_item": {
 *     "name": "Bohemian Rhapsody",
 *     "artists": [{"name": "Queen"}],
 *     "album": {"name": "A Night at the Opera", "image": {"path": "/api/thumb/..."}},
 *     "duration": 354.0,
 *     "media_type": "track"
 *   },
 *   "elapsed_time": 45.2,
 *   "shuffle_enabled": false,
 *   "repeat_mode": "off"
 * }
 */
#include "RestClient.h"
#include "../AppConfig.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// ── init ─────────────────────────────────────────────────────────────
void RestClient::begin(const String& serverIP, uint16_t port,
                       const String& token) {
    _base  = "http://" + serverIP + ":" + String(port) + MA_REST_BASE;
    _token = token;
    LOG_I("REST", "Base URL: %s  auth=%s", _base.c_str(),
          _token.isEmpty() ? "none" : "token");
}

// ── Private helpers ───────────────────────────────────────────────────
String RestClient::_makeUrl(const String& path) const {
    return _base + path;
}

void RestClient::_addAuth(HTTPClient& http) const {
    if (!_token.isEmpty())
        http.addHeader(MA_AUTH_HEADER, _token);
}

String RestClient::_get(const String& url) {
    if (!WiFi.isConnected()) return "";
    HTTPClient http;
    http.setTimeout(_timeout);
    http.begin(url);
    _addAuth(http);
    http.addHeader("Accept", "application/json");
    int code = http.GET();
    if (code == 200) {
        String body = http.getString();
        http.end();
        return body;
    }
    LOG_W("REST", "GET %s → %d", url.c_str(), code);
    http.end();
    return "";
}

int RestClient::_post(const String& url, const String& body) {
    if (!WiFi.isConnected()) return -1;
    HTTPClient http;
    http.setTimeout(_timeout);
    http.begin(url);
    _addAuth(http);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    LOG_I("REST", "POST %s → %d", url.c_str(), code);
    http.end();
    return code;
}

// ── JSON parsing helpers ──────────────────────────────────────────────
QueueItem RestClient::_parseQueueItem(const JsonObject& obj) {
    QueueItem q;
    q.title    = obj["name"].as<String>();
    q.duration = obj["duration"] | 0.0f;

    // Artists array → join with comma
    JsonArray artists = obj["artists"].as<JsonArray>();
    for (JsonObject a : artists) {
        if (!q.artist.isEmpty()) q.artist += ", ";
        q.artist += a["name"].as<String>();
    }

    JsonObject album = obj["album"].as<JsonObject>();
    if (!album.isNull()) {
        q.album = album["name"].as<String>();
        JsonObject img = album["image"].as<JsonObject>();
        if (!img.isNull())
            q.imageUrl = img["path"].as<String>();
    }
    return q;
}

PlayerSnapshot RestClient::_parsePlayer(const JsonObject& obj) {
    PlayerSnapshot s;
    s.playerId    = obj["player_id"].as<String>();
    s.playerName  = obj["display_name"].as<String>();
    s.volume      = obj["volume_level"] | 50;
    s.muted       = obj["muted"] | false;
    s.elapsedTime = obj["elapsed_time"] | 0.0f;
    s.shuffled    = obj["shuffle_enabled"] | false;
    s.repeatMode  = obj["repeat_mode"] | String("off");

    String stateStr = obj["state"] | String("idle");
    if (stateStr == "playing")      s.state = PlayState::PLAYING;
    else if (stateStr == "paused")  s.state = PlayState::PAUSED;
    else if (stateStr == "idle")    s.state = PlayState::IDLE;
    else if (stateStr == "off")     s.state = PlayState::OFF;
    else                            s.state = PlayState::UNKNOWN;

    JsonObject ci = obj["current_item"].as<JsonObject>();
    if (!ci.isNull())
        s.currentItem = _parseQueueItem(ci);

    s.lastUpdated = millis();
    return s;
}

// ── Public API ────────────────────────────────────────────────────────
bool RestClient::fetchPlayers(PlayerListCb onDone, ErrorCb onErr) {
    String body = _get(_makeUrl("/players"));
    if (body.isEmpty()) {
        if (onErr) onErr("No response from server");
        return false;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        if (onErr) onErr(String("JSON error: ") + err.c_str());
        return false;
    }

    std::vector<PlayerSnapshot> players;
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject p : arr)
        players.push_back(_parsePlayer(p));

    LOG_I("REST", "Fetched %d players", players.size());
    if (onDone) onDone(players);
    return true;
}

bool RestClient::fetchPlayerState(const String& playerId,
                                  PlayerUpdateCb onDone, ErrorCb onErr) {
    String body = _get(_makeUrl("/players/" + playerId));
    if (body.isEmpty()) {
        if (onErr) onErr("No response");
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        if (onErr) onErr(String("JSON: ") + err.c_str());
        return false;
    }

    PlayerSnapshot snap = _parsePlayer(doc.as<JsonObject>());
    LOG_I("REST", "Player state: %s | %s | vol=%d",
          playerId.c_str(), snap.currentItem.title.c_str(), snap.volume);
    if (onDone) onDone(snap);
    return true;
}

// ── Commands ──────────────────────────────────────────────────────────
int RestClient::cmdPlay(const String& pid) {
    return _post(_makeUrl("/players/" + pid + "/cmd/" MA_CMD_PLAY));
}
int RestClient::cmdPause(const String& pid) {
    return _post(_makeUrl("/players/" + pid + "/cmd/" MA_CMD_PAUSE));
}
int RestClient::cmdNext(const String& pid) {
    return _post(_makeUrl("/players/" + pid + "/cmd/" MA_CMD_NEXT));
}
int RestClient::cmdPrevious(const String& pid) {
    return _post(_makeUrl("/players/" + pid + "/cmd/" MA_CMD_PREVIOUS));
}
int RestClient::cmdVolumeSet(const String& pid, int vol) {
    vol = constrain(vol, 0, 100);
    return _post(_makeUrl("/players/" + pid + "/cmd/" MA_CMD_VOL_SET
                          "?volume_level=" + String(vol)));
}
int RestClient::cmdStop(const String& pid) {
    return _post(_makeUrl("/players/" + pid + "/cmd/" MA_CMD_STOP));
}

// ── Album art ─────────────────────────────────────────────────────────
bool RestClient::fetchAlbumArt(const String& imageUrl,
                               uint8_t** outBuf, size_t* outLen) {
    if (imageUrl.isEmpty()) return false;
    if (!WiFi.isConnected()) return false;

    // MA returns thumbnail via path like /api/thumb/...?size=128
    // We request a small size to save memory
    String url = imageUrl;
    if (url.startsWith("/")) {
        // Relative path — prepend server base (strip /api suffix)
        String serverBase = _base;
        serverBase.replace(MA_REST_BASE, "");
        url = serverBase + url;
    }
    // Request small thumbnail
    if (url.indexOf("size=") < 0) url += "?size=80";

    HTTPClient http;
    http.setTimeout(8000);
    http.begin(url);
    _addAuth(http);
    int code = http.GET();
    if (code != 200) {
        LOG_W("REST", "Art fetch %d from %s", code, url.c_str());
        http.end();
        return false;
    }

    int len = http.getSize();
    if (len <= 0 || len > 32768) {   // refuse > 32 KB
        http.end();
        return false;
    }

    *outBuf = (uint8_t*)malloc(len);
    if (!(*outBuf)) { http.end(); return false; }

    WiFiClient* stream = http.getStreamPtr();
    size_t      got    = 0;
    while (http.connected() && got < (size_t)len) {
        if (stream->available()) {
            (*outBuf)[got++] = stream->read();
        }
    }
    *outLen = got;
    http.end();

    LOG_I("REST", "Art fetched %d bytes", got);
    return (got > 0);
}
