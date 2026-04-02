#pragma once
/**
 * api/WebSocketClient.h
 *
 * Music Assistant WebSocket (v2.x):
 *   URL   : ws://<ip>:<port>/api/ws
 *   Auth  : query param  ?token=<apikey>  OR header x-api-key
 *
 * Protocol after connect:
 *   → send: {"command": "start_listening"}
 *   ← recv: {"event": "...", "object_id": "...", "data": {...}}
 *
 * Events we handle:
 *   player_updated   → update player snapshot
 *   queue_updated    → update current track
 *   player_settings_updated → shuffle/repeat
 *
 * Commands via WebSocket (alternative to REST):
 *   {"command":"player_command","player_id":"...","cmd":"play"}
 */
#include <Arduino.h>
#include "../state/PlayerState.h"

// Forward declarations
class WebSocketsClient;
class UIManager;

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    void begin(const String& serverIP, uint16_t port,
               const String& token,
               PlayerState*  playerState,
               UIManager*    uiManager);

    void connect();
    void disconnect();
    void loop();

    bool isConnected() const { return _connected; }

private:
    WebSocketsClient* _ws       = nullptr;
    PlayerState*      _state    = nullptr;
    UIManager*        _ui       = nullptr;
    String            _serverIP;
    uint16_t          _port     = 8095;
    String            _token;
    bool              _connected= false;
    uint32_t          _lastPing = 0;
    uint32_t          _lastReconnectAttempt = 0;

    void _onEvent(uint8_t type, uint8_t* payload, size_t len);
    void _handleMessage(const String& msg);
    void _sendStartListening();
    void _parsePlayerUpdated(const JsonObject& data);
    void _parseQueueUpdated(const JsonObject& data);
};
