# Music Assistant API Reference
## (Assumed schema for v2.x — documented assumptions)

---

## REST Endpoints

Base URL: `http://<ip>:8095/api`
Auth header: `x-api-key: <token>` (optional — only if MA requires auth)

---

### GET /api/players
Returns array of all player objects.

```json
[
  {
    "player_id": "cast_6c7e4c8b_living_room",
    "display_name": "Living Room",
    "powered": true,
    "available": true,
    "state": "playing",
    "volume_level": 72,
    "muted": false,
    "shuffle_enabled": false,
    "repeat_mode": "off",
    "elapsed_time": 45.2,
    "elapsed_time_last_updated": 1706000000.0,
    "current_item": {
      "item_id": "spotify://track/4uLU6hMCjMI75M1A2tKUQC",
      "name": "Bohemian Rhapsody",
      "duration": 354.0,
      "media_type": "track",
      "artists": [
        { "item_id": "spotify://artist/1dfeR4HaWDbWqFHLkxsg1d", "name": "Queen" }
      ],
      "album": {
        "item_id": "spotify://album/6i6folBtxKV28WX3msQ4FE",
        "name": "A Night at the Opera",
        "image": {
          "type": "image",
          "path": "/api/thumb/spotify%3A%2F%2Falbum%2F6i6folBtxKV28WX3msQ4FE?size=128",
          "provider": "builtin"
        }
      }
    }
  }
]
```

---

### GET /api/players/{player_id}
Returns single player object (same schema as above, unwrapped from array).

---

### POST /api/players/{player_id}/cmd/play
### POST /api/players/{player_id}/cmd/pause
### POST /api/players/{player_id}/cmd/next
### POST /api/players/{player_id}/cmd/previous
### POST /api/players/{player_id}/cmd/stop

No request body required. Returns HTTP 200 on success.

---

### POST /api/players/{player_id}/cmd/volume_set?volume_level=50

Query param: `volume_level` (integer 0–100)
Returns HTTP 200 on success.

---

### GET /api/thumb/{encoded_image_path}?size=80

Returns JPEG image bytes.
- `size` query param controls thumbnail size (pixels)
- Recommended: 80 for the CYD's 130×130 art panel

---

## WebSocket

URL: `ws://<ip>:8095/api/ws`
Optional auth: `ws://<ip>:8095/api/ws?token=<apikey>`

### Connection sequence

After connecting, send:
```json
{"command": "start_listening"}
```

Server begins sending events:

---

### Event: player_updated
```json
{
  "event": "player_updated",
  "object_id": "cast_6c7e4c8b_living_room",
  "data": {
    "player_id": "cast_6c7e4c8b_living_room",
    "display_name": "Living Room",
    "state": "playing",
    "volume_level": 72,
    "muted": false,
    "shuffle_enabled": false,
    "repeat_mode": "off",
    "elapsed_time": 46.8,
    "current_item": { "...": "same as REST" }
  }
}
```

`state` values: `playing` | `paused` | `idle` | `off`

---

### Event: queue_updated
```json
{
  "event": "queue_updated",
  "object_id": "cast_6c7e4c8b_living_room",
  "data": {
    "queue_id": "cast_6c7e4c8b_living_room",
    "current_item": { "...": "track object" },
    "next_item": { "...": "track object or null" }
  }
}
```

---

### Event: players_updated
Bulk update — same as player_updated but `data` is an array of player objects.

---

## Assumptions Made

1. **Auth is optional**: MA v2 may require an API token — the firmware sends it
   via both the `x-api-key` header and a `?token=` WS query param.

2. **Player ID stability**: Player IDs are stable across server restarts.

3. **Thumbnail path**: Album art is served as a relative path under `/api/thumb/...`
   which the firmware prefixes with `http://<ip>:<port>`.

4. **WS start command**: `{"command": "start_listening"}` subscribes to all events.
   Some MA versions may require specifying event types; adapt as needed.

5. **elapsed_time**: Updated in `player_updated` events; between events the
   firmware advances the counter locally for smooth progress bar animation.

6. **Port**: Default is 8095. Configurable via settings screen.

7. **Volume**: 0–100 integer in both REST and WS payloads.

---

## Troubleshooting

| Symptom | Check |
|---------|-------|
| "No players found" | Verify server IP:port, check MA is running |
| Album art not loading | Check `/api/thumb/` path format in your MA version |
| WS disconnects | Increase `WS_RECONNECT_MS` in AppConfig.h |
| Touch mis-calibrated | Adjust `x_min/max`, `y_min/max` in DisplaySetup.cpp |
| Flickering UI | Ensure `LV_COLOR_16_SWAP 1` in lv_conf.h |
