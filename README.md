# Remote Switch

Wireless home-appliance control with **2 ESP boards** talking plain HTTP over
home WiFi. No ESP-NOW, no RF transmitter — each board hosts the same
"Home Control" web dashboard, drives its own hardware natively, and proxies
the other board's hardware through a tiny REST API.

## Architecture — 2 ESPs, symmetric HTTP

```
  433 MHz remote ──┐
                   ▼
            ┌─────────────┐  HTTP (home WiFi)   ┌──────────────┐
            │ ESP 3       │◄───────────────────►│ ESP 1        │
            │ ServerBoard │  /api/cooler/on     │ RelayBoard   │
            │ (ESP32)     │  /api/strips/toggle │ (ESP8266)    │
            └──────┬──────┘                     └──────┬───────┘
                   │ strips relay                    │ 2-channel relays
                   │ (boolean on/off)                │ (active-low)
                   ▼                                 ▼
              Strip lights                     Cooler / Light

  phone ──► http://192.168.0.189/ (ESP 3 dashboard)
  phone ──► http://192.168.0.145/ (ESP 1 dashboard — same UI)
```

- **ESP 1 (RelayBoard, ESP8266, 192.168.0.145):** drives the cooler + light
  relays natively. Its dashboard toggles them directly; the Strips card is
  proxied to ESP 3 via `GET /api/strips/toggle`.
- **ESP 3 (ServerBoard, ESP32, 192.168.0.189):** drives the strips relay
  natively and owns the 433 MHz RF receiver. Remote cooler/light buttons are
  forwarded to ESP 1's API (`/api/cooler/on` …); the remote strips button
  toggles locally. Its dashboard proxies cooler/light to ESP 1.
- **State sync:** each board's `/api/state` returns *merged* state — its own
  hardware plus the peer's live state (queried over HTTP, 1.5 s throttle).
  Both dashboards auto-refresh every 2 s and show a peer online/offline dot.

## Nodes

| Folder | ESP | Role |
|---|---|---|
| `RelayBoard/` | ESP 1 | ESP8266 + WiFi webserver + 2-channel relay board (active-low). Native: cooler/light. Proxied: strips |
| `ServerBoard/` | ESP 3 | ESP32 + WiFi webserver + strips relay + 433 MHz RF receiver. Native: strips. Proxied: cooler/light |
| `ButtonController/` | (legacy) | Old ESP-NOW FLASH-button toggle — kept for reference, not part of the current setup |
| `tools/GetMACAddress/` | — | Prints a board's MAC address (no longer needed — boards talk over HTTP now) |

## REST API (same shape on both boards)

Each board natively serves its own devices; the other device is proxied:

```
GET  /                        Home Control dashboard (HTML)
GET  /api/state               merged JSON: {"cooler":..,"light":..,"strips":..,"peer_online":..}
GET  /toggle?dev=cooler|light|strips
GET  /api/<dev>/on | /off | /toggle | /state     (dev = cooler|light on ESP 1, strips on ESP 3)
```

Examples: `http://192.168.0.145/api/cooler/on`,
`http://192.168.0.189/api/strips/toggle`.

## Hardware

- 1× ESP8266 board, e.g. NodeMCU (ESP 1 RelayBoard)
- 1× ESP32 board, e.g. ESP32 DevKit (ESP 3 ServerBoard)
- 1× 2-channel relay module (**active-low**) for ESP 1 (cooler + light)
- 1× relay module for the strip lights on ESP 3
- 1× 433 MHz RF receiver module + RF remote (ESP 3)

**Board selection in Arduino IDE:** "NodeMCU 1.0 (ESP-12E Module)" for ESP 1,
"ESP32 Dev Module" for ESP 3.

## Setup

1. **Edit configs** (both `config.h` files are git-ignored — passwords never
   leave your machine):
   - `RelayBoard/config.h` — copy from `config.example.h`; fill `WIFI_SSID` /
     `WIFI_PASS`; check `COOLER_PIN` / `LIGHT_PIN` mapping and `PEER_IP`.
   - `ServerBoard/config.h` — copy from `config.example.h`; fill `WIFI_SSID` /
     `WIFI_PASS`; check `STRIP_PIN`, `STRIP_ACTIVE_LOW`, `RF_RX_PIN`, `PEER_IP`,
     and the RF code mapping.
2. **Flash**: `RelayBoard` on ESP 1, `ServerBoard` on ESP 3.
3. Open **http://192.168.0.145/** or **http://192.168.0.189/** — the
   **Home Control** dashboard appears (Cooler / Light / Strips cards with
   ON/OFF buttons, auto-refreshing, peer status dot).

`ServerBoard` needs the [RCSwitch](https://github.com/sui77/rc-switch) library.

## 433 MHz remote codes (received by ESP 3)

| Code | Action |
|---|---|
| 8355012 / 8355020 | Cooler ON / OFF (forwarded to ESP 1) |
| 8355010 / 8355016 | Light ON / OFF (forwarded to ESP 1) |
| _(unknown)_ | Strips toggle (code missing — press the button, read `RF code: <n>` from ESP 3's serial monitor, put it in `RF_CODE_STRIPS_TOGGLE`) |

## Troubleshooting

- **Peer shows OFFLINE** → both boards on home WiFi? Static IPs (`192.168.0.145`
  / `.189`) not clashing with another device? Check each board's serial monitor.
- **Relay does the opposite** → flip `RELAY_ACTIVE_LOW` (ESP 1) or
  `STRIP_ACTIVE_LOW` (ESP 3) in `config.h`.
- **RF remote does nothing** → open ESP 3's serial monitor: unknown codes get
  printed with a hint. Add them to `ServerBoard/config.h`.
- **Web toggle does nothing for the peer's device** → the peer board may be
  down; the dashboard's peer dot tells you. Peer HTTP timeout is 800 ms.
