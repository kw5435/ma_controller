/**
 * api/WebSocketClient.cpp
 */
#include "WebSocketClient.h"
#include "../AppConfig.h"
#include "../ui/UIManager.h"
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// ── Static trampoline (WebSockets lib needs C-style callback) ─────────
static WebSocketClient* _instance = nullptr;

static void _wsEventStatic(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) _instance->_onEvent((uint8_t)type, payload, length);
}

// ── Constructor / Destructor ──────────────────────────────────────────
WebSocketClient::WebSocketClient() {
    _ws = new WebSocketsClient();
    _instance = this;
}
WebSocketClient::~WebSocketClient() {
    delete _ws;
}

// ── begin ─────────────────────────────────────────────────────────────
void WebSocketClient::begin(const String& serverIP, uint16_t port,
                            const String& token,
                            PlayerState*  playerState,
                            UIManager*    uiManager) {
    _serverIP = serverIP;
    _port     = port;
    _token    = token;
    _state    = playerState;
    _ui       = uiManager;

    // Extra headers for auth
    if (!_token.isEmpty()) {
        _ws->setExtraHeaders((String(MA_AUTH_HEADER ": ") + _token).c_str());
    }

    // Reconnect automatically every WS_RECONNECT_MS
    _ws->setReconnectInterval(WS_RECONNECT_MS);

    LOG_I("WS", "Configured for %s:%d", serverIP.c_str(), port);
}

void WebSocketClient::connect() {
    if (!WiFi.isConnected()) {
        LOG_W("WS", "WiFi not connected — skip");
        return;
    }
    String path = String(MA_WS_PATH);
    if (!_token.isEmpty())
        path += "?token=" + _token;

    _ws->begin(_serverIP.c_str(), _port, path.c_str());
    _ws->onEvent(_wsEventStatic);
    LOG_I("WS", "Connecting to ws://%s:%d%s", _serverIP.c_str(), _port, path.c_str());
}

void WebSocketClient::disconnect() {
    _ws->disconnect();
    _connected = false;
}

// ── loop ─────────────────────────────────────────────────────────────
void WebSocketClient::loop() {
    _ws->loop();

    // Keep-alive ping every 30 s
    if (_connected && (millis() - _lastPing > 30000)) {
        _ws->sendPing();
        _lastPing = millis();
    }
}

// ── Event handler ─────────────────────────────────────────────────────
void WebSocketClient::_onEvent(uint8_t type, uint8_t* payload, size_t len) {
    switch ((WStype_t)type) {

    case WStype_CONNECTED:
        _connected = true;
        LOG_I("WS", "Connected");
        _sendStartListening();
        if (_ui) _ui->setConnectionIcon(true);
        break;

    case WStype_DISCONNECTED:
        _connected = false;
        LOG_W("WS", "Disconnected — will retry");
        if (_ui) _ui->setConnectionIcon(false);
        break;

    case WStype_TEXT:
        if (payload && len > 0) {
            _handleMessage(String((char*)payload));
        }
        break;

    case WStype_PING:
        LOG_I("WS", "Ping received");
        break;

    case WStype_ERROR:
        LOG_E("WS", "Error (type %d)", type);
        break;

    default:
        break;
    }
}

// ── Subscribe ─────────────────────────────────────────────────────────
void WebSocketClient::_sendStartListening() {
    // MA v2 WebSocket expects this to begin receiving events
    String msg = R"({"command":"start_listening"})";
    _ws->sendTXT(msg);
    LOG_I("WS", "Sent start_listening");
}

// ── Message dispatch ──────────────────────────────────────────────────
void WebSocketClient::_handleMessage(const String& msg) {
    // MA WS message envelope:
    // {"event": "player_updated", "object_id": "...", "data": {...}}
    // OR direct player object as data

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, msg);
    if (err) {
        LOG_W("WS", "JSON err: %s  raw=%s", err.c_str(), msg.substring(0,80).c_str());
        return;
    }

    String event = doc["event"] | String("");

    LOG_I("WS", "Event: %s", event.c_str());

    if (event == "player_updated") {
        JsonObject data = doc["data"].as<JsonObject>();
        _parsePlayerUpdated(data);
    }
    else if (event == "queue_updated") {
        JsonObject data = doc["data"].as<JsonObject>();
        _parseQueueUpdated(data);
    }
    else if (event == "players_updated") {
        // Array of all players — handle as bulk update
        JsonArray arr = doc["data"].as<JsonArray>();
        for (JsonObject p : arr) {
            // Only update if it matches our stored player
            String pid = p["player_id"] | String("");
            if (pid == _state->getPlayerId() || _state->getPlayerId().isEmpty()) {
                _parsePlayerUpdated(p);
                break;
            }
        }
    }
}

// ── Player update parsing ─────────────────────────────────────────────
void WebSocketClient::_parsePlayerUpdated(const JsonObject& data) {
    if (data.isNull() || !_state) return;

    // Build snapshot incrementally
    PlayerSnapshot s;
    s.playerId   = data["player_id"]    | _state->getPlayerId().c_str();
    s.playerName = data["display_name"] | String("");
    s.volume     = data["volume_level"] | _state->getVolume();
    s.muted      = data["muted"]        | false;
    s.elapsedTime= data["elapsed_time"] | _state->getElapsed();
    s.shuffled   = data["shuffle_enabled"] | false;
    s.repeatMode = data["repeat_mode"]  | String("off");
    s.lastUpdated= millis();

    String stateStr = data["state"] | String("idle");
    if      (stateStr == "playing") s.state = PlayState::PLAYING;
    else if (stateStr == "paused")  s.state = PlayState::PAUSED;
    else if (stateStr == "idle")    s.state = PlayState::IDLE;
    else if (stateStr == "off")     s.state = PlayState::OFF;
    else                            s.state = PlayState::UNKNOWN;

    // Current track
    JsonObject ci = data["current_item"].as<JsonObject>();
    if (!ci.isNull()) {
        s.currentItem.title    = ci["name"] | String("");
        s.currentItem.duration = ci["duration"] | 0.0f;

        JsonArray artists = ci["artists"].as<JsonArray>();
        for (JsonObject a : artists) {
            if (!s.currentItem.artist.isEmpty()) s.currentItem.artist += ", ";
            s.currentItem.artist += a["name"].as<String>();
        }

        JsonObject album = ci["album"].as<JsonObject>();
        if (!album.isNull()) {
            s.currentItem.album = album["name"] | String("");
            JsonObject img = album["image"].as<JsonObject>();
            if (!img.isNull())
                s.currentItem.imageUrl = img["path"] | String("");
        }
    }

    _state->applySnapshot(s);

    LOG_I("WS", "Player updated: %s | %s | vol=%d",
          stateStr.c_str(), s.currentItem.title.c_str(), s.volume);
}

void WebSocketClient::_parseQueueUpdated(const JsonObject& data) {
    if (data.isNull() || !_state) return;

    JsonObject item = data["current_item"].as<JsonObject>();
    if (item.isNull()) return;

    QueueItem q;
    q.title    = item["name"] | String("");
    q.duration = item["duration"] | 0.0f;

    JsonArray artists = item["artists"].as<JsonArray>();
    for (JsonObject a : artists) {
        if (!q.artist.isEmpty()) q.artist += ", ";
        q.artist += a["name"].as<String>();
    }
    JsonObject album = item["album"].as<JsonObject>();
    if (!album.isNull()) {
        q.album = album["name"] | String("");
        JsonObject img = album["image"].as<JsonObject>();
        if (!img.isNull()) q.imageUrl = img["path"] | String("");
    }

    _state->applyTrack(q);
}
