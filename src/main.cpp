/**
 * main.cpp
 *
 * Music Assistant CYD Controller
 * Orchestration layer — wires all modules together.
 *
 * Boot sequence:
 *  1. NVS load settings
 *  2. Display + LVGL init → show loading screen
 *  3a. If no config → start captive portal
 *  3b. If config → connect WiFi
 *  4. Connect to MA WebSocket
 *  5. Fetch initial player state via REST
 *  6. Show player screen
 *  7. Main loop: LVGL tick + WS loop + state tick
 */
#include <Arduino.h>
#include "AppConfig.h"
#include "display/DisplaySetup.h"
#include "storage/Storage.h"
#include "state/PlayerState.h"
#include "network/WiFiManager.h"
#include "api/RestClient.h"
#include "api/WebSocketClient.h"
#include "ui/UIManager.h"

// ── Module instances ───────────────────────────────────────────────────
static Storage         storage;
static PlayerState     playerState;
static WiFiManager     wifiMgr;
static RestClient      restClient;
static WebSocketClient wsClient;
static UIManager       uiManager;

// ── Timing ────────────────────────────────────────────────────────────
static uint32_t lastLvglTick    = 0;
static uint32_t lastStateTick   = 0;
static uint32_t lastFallbackPoll= 0;
static uint32_t lastWsCheck     = 0;

// ── Forward declarations ──────────────────────────────────────────────
static void startCaptivePortal();
static void startWifiConnect(const AppSettings& s);
static void onWifiConnected(const AppSettings& s);
static void onWifiFailed();
static void connectToMA(const AppSettings& s);
static void fetchInitialState(const AppSettings& s);

// ── LVGL tick source ───────────────────────────────────────────────────
static void lvglTick() {
    uint32_t now = millis();
    if (now - lastLvglTick >= 5) {
        lv_tick_inc(now - lastLvglTick);
        lastLvglTick = now;
    }
    lv_task_handler();
}

// ─────────────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    LOG_I("MAIN", "=== MA Controller boot ===");
    LOG_I("MAIN", "Free heap: %d KB", ESP.getFreeHeap() / 1024);

    // ── Status LED init ───────────────────────────────────────────
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    // Active LOW — all off
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, HIGH);

    // ── NVS ───────────────────────────────────────────────────────
    storage.begin();
    AppSettings settings = storage.loadSettings();

    // ── Display + LVGL ────────────────────────────────────────────
    DisplaySetup::init();
    DisplaySetup::setBrightness(settings.brightness);

    // ── UI Manager ────────────────────────────────────────────────
    uiManager.begin(&storage, &playerState, &restClient, &wsClient, &wifiMgr);
    uiManager.showLoadingScreen("Initialising...");
    lvglTick();

    // ── WiFi setup ────────────────────────────────────────────────
    wifiMgr.begin();

    if (!settings.configured) {
        LOG_W("MAIN", "No config found — starting captive portal");
        uiManager.showStatus("No config — connect to\n'" PORTAL_SSID "'");
        startCaptivePortal();
    } else {
        startWifiConnect(settings);
    }
}

// ─────────────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────────────
void loop() {
    // ── LVGL tick (must be fast) ──────────────────────────────────
    lvglTick();

    // ── WiFi watchdog ─────────────────────────────────────────────
    wifiMgr.loop();

    // ── WebSocket loop ────────────────────────────────────────────
    wsClient.loop();

    // ── State tick (advance elapsed time locally) ─────────────────
    uint32_t now = millis();
    if (now - lastStateTick >= PROGRESS_UPDATE_MS) {
        playerState.tick();
        lastStateTick = now;
    }

    // ── Fallback REST poll when WS is disconnected ────────────────
    if (!wsClient.isConnected() && wifiMgr.isConnected()) {
        // Try to reconnect WS periodically
        if (now - lastWsCheck >= WS_RECONNECT_MS) {
            lastWsCheck = now;
            LOG_W("MAIN", "WS disconnected — attempting reconnect");
            wsClient.connect();
        }

        // Poll REST as backup
        if (now - lastFallbackPoll >= FALLBACK_POLL_MS) {
            lastFallbackPoll = now;
            String pid = playerState.getPlayerId();
            if (!pid.isEmpty()) {
                restClient.fetchPlayerState(pid,
                    [](PlayerSnapshot snap) { playerState.applySnapshot(snap); },
                    nullptr);
            }
        }
    }

    // ── UI redraw ─────────────────────────────────────────────────
    uiManager.update();
}

// ─────────────────────────────────────────────────────────────────────
//  Network helpers
// ─────────────────────────────────────────────────────────────────────
static void startCaptivePortal() {
    // Blue LED = portal active
    digitalWrite(PIN_LED_B, LOW);
    wifiMgr.startCaptivePortal(PORTAL_SSID);
    uiManager.showLoadingScreen(
        "Setup mode\nConnect to WiFi:\n" PORTAL_SSID "\nThen open 192.168.4.1");
}

static AppSettings _pendingSettings;  // captured for callbacks

static void startWifiConnect(const AppSettings& s) {
    _pendingSettings = s;
    uiManager.showStatus("Connecting to " + s.ssid + "...");

    // Red LED = connecting
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, HIGH);

    wifiMgr.connect(s.ssid, s.password, WIFI_TIMEOUT_MS,
        /* onConnected */ []() { onWifiConnected(_pendingSettings); },
        /* onFailed    */ []() { onWifiFailed(); }
    );
}

static void onWifiConnected(const AppSettings& s) {
    LOG_I("MAIN", "WiFi OK → %s", wifiMgr.localIP().c_str());

    // Green LED = connected
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, LOW);

    uiManager.setConnectionIcon(true);
    uiManager.showStatus("WiFi connected\nReaching MA server...");

    connectToMA(s);
}

static void onWifiFailed() {
    LOG_E("MAIN", "WiFi failed");
    // Red LED = error
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, HIGH);

    uiManager.showStatus("WiFi failed!\nCheck settings or\nreset device.");
    uiManager.setConnectionIcon(false);

    // After 10s show settings so user can re-enter credentials
    delay(3000);
    uiManager.showSettingsScreen();
}

static void connectToMA(const AppSettings& s) {
    // Init REST client
    restClient.begin(s.serverIP, s.serverPort, s.apiToken);

    // Determine player ID
    String pid = s.playerId;

    if (pid.isEmpty()) {
        // Fetch player list and use first one
        uiManager.showStatus("Fetching players...");
        bool ok = restClient.fetchPlayers(
            [&](std::vector<PlayerSnapshot> players) {
                if (!players.empty()) {
                    pid = players[0].playerId;
                    storage.savePlayerId(pid);
                    LOG_I("MAIN", "Auto-selected player: %s", pid.c_str());

                    // Update uiMgr player list too
                    uiManager.onPlayerListReceived(players);
                }
            }, nullptr);

        if (!ok || pid.isEmpty()) {
            uiManager.showStatus("No MA players found!\nCheck server IP.");
            LOG_E("MAIN", "No players — check MA server config");
            delay(4000);
            uiManager.showSettingsScreen();
            return;
        }
    }

    // Fetch initial full state
    fetchInitialState(s);

    // Init WebSocket
    wsClient.begin(s.serverIP, s.serverPort, s.apiToken,
                   &playerState, &uiManager);
    wsClient.connect();

    // Show player screen
    uiManager.showPlayerScreen();
    LOG_I("MAIN", "Startup complete. Free heap: %d KB", ESP.getFreeHeap() / 1024);
}

static void fetchInitialState(const AppSettings& s) {
    String pid = storage.loadSettings().playerId;
    if (pid.isEmpty()) return;

    restClient.fetchPlayerState(pid,
        [](PlayerSnapshot snap) {
            playerState.applySnapshot(snap);
            LOG_I("MAIN", "Initial state: %s | vol=%d",
                  snap.currentItem.title.c_str(), snap.volume);
        },
        [](String err) {
            LOG_E("MAIN", "Failed to fetch state: %s", err.c_str());
        }
    );
}
