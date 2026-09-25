# RemoteSw — 2-Board Home Automation

Two ESP boards, one shared dashboard. Cooler + light on ESP 1, LED strips on ESP 3, all controllable from either board's web UI, REST API, or a 433&nbsp;MHz remote.

![Platform: ESP8266](https://img.shields.io/badge/ESP1-ESP8266-blue)
![Platform: ESP32](https://img.shields.io/badge/ESP3-ESP32-green)

## Architecture

```text
                    ┌──────────────┐  HTTP (Wi-Fi)  ┌──────────────┐
                    │   ESP 1      │◄──────────────►│    ESP 3     │
                    │  ESP8266     │                │    ESP32     │
                    │  RelayBoard  │                │  ServerBoard │
                    └──────┬───────┘                └──────┬───────┘
                           │ relay 1 (D5)                 │ GPIO 12 ──► strips relay
                           │ relay 2 (D6)                 │ GPIO 14 ◄── 433 MHz receiver
                           ▼                              ▼
                     cooler + light                  LED strips
                                                          ▲
                                               433 MHz remote ──┘
```

- **ESP 1** natively drives the cooler + light relays. Strips commands are proxied to ESP 3 over HTTP.
- **ESP 3** natively drives the strips relay and owns the 433&nbsp;MHz RF receiver. Cooler/light remote buttons are forwarded to ESP 1 over HTTP; the strips button toggles the local relay directly.
- Both boards serve the **same Home Control dashboard** and the **same REST API**. Each `/api/state` merges local + peer state, and the dashboard shows whether the peer board is online.
- Cross-board commands need both boards on the same Wi-Fi network. Local controls (ESP 1 → cooler/light, ESP 3 → strips) work even if the peer is offline.

> `RelayBoard/` and `ServerBoard/` are older copies of the same sketches, superseded by `esp1/` and `esp3/`. Flash from `esp1/` and `esp3/`.

## Hardware

| Board | Chip | IP | Natively controls |
|-------|------|----|-------------------|
| ESP 1 (`esp1/`) | ESP8266 (NodeMCU) | DHCP (check Serial Monitor) | Cooler relay, Light relay |
| ESP 3 (`esp3/`) | ESP32 Dev Module | Static `192.168.0.189` | Strips relay, 433 MHz receiver |

### ESP 1 pinout

| Relay | Pin | Appliance (unconfirmed — verify on your wiring) |
|-------|-----|--------------------------------------------------|
| Relay 1 | D5 | Cooler (?) |
| Relay 2 | D6 | Light (?) |

Relays are **active-low** (`LOW` = ON).

### ESP 3 pinout

| Function | GPIO | Notes |
|----------|------|-------|
| Strips relay | 12 | **Active-high** (`HIGH` = ON) |
| 433 MHz receiver data | 14 | Use the module's **DATA** pin (not NC). Receiver likes 5V (VIN); 3.3V is often too weak |

### 433 MHz remote codes

| Button | Code | Action |
|--------|------|--------|
| Cooler ON | `8355012` | via ESP 1 API |
| Cooler OFF | `8355020` | via ESP 1 API |
| Light ON | `8355010` | via ESP 1 API |
| Light OFF | `8355016` | via ESP 1 API |
| Strips toggle | `8355009` | local relay toggle |

Any received code is printed to the Serial Monitor as `RF code: <number>`, so new remotes can be learned by just pressing buttons and watching the output.

## Flashing

**ESP 1** — `esp1/esp1.ino`
- Board: **NodeMCU 1.0 (ESP-12E Module)**
- Libraries: none extra (ESP8266 core only)

**ESP 3** — `esp3/esp3.ino`
- Board: **ESP32 Dev Module**
- Library: **RCSwitch** (Arduino Library Manager)

Steps per board:

1. Copy `config.example.h` → `config.h` in the same folder.
2. Fill in your Wi-Fi SSID/password (and ESP 3's peer IP — see note below).
3. Select the board, compile, flash.
4. Open Serial Monitor at **115200 baud** to see the dashboard URL and RF codes.

> **Note:** ESP 1 uses DHCP, so its IP can change. ESP 3 reaches it via `PEER_IP` in `esp3/config.h` — if cooler/light control *from ESP 3* stops working, check ESP 1's Serial Monitor for its current IP and update `PEER_IP`.

`config.h` files are git-ignored — real credentials never get committed. Only `config.example.h` templates live in the repo.

## Dashboards & API

Open in a browser (same Wi-Fi network):

- ESP 1: `http://<esp1-ip>/` (IP printed in Serial Monitor, e.g. `http://192.168.0.132/`)
- ESP 3: `http://192.168.0.189/`

REST API (identical on both boards; non-native devices are proxied to the peer):

```text
GET  /
GET  /api/state
GET  /toggle?dev=cooler|light|strips
GET  /api/<dev>/on
GET  /api/<dev>/off
GET  /api/<dev>/toggle
GET  /api/<dev>/state
```

Examples:

```text
http://192.168.0.132/api/cooler/on
http://192.168.0.189/api/strips/toggle
```

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| Garbage characters in Serial Monitor | Baud rate mismatch — set monitor to **115200** |
| Board reboots when opening Serial Monitor | Normal — DTR reset on monitor open |
| `WiFi connecting........` never ends | Wrong SSID/password, or router's 2.4 GHz band is off (ESP8266 can't see 5 GHz) |
| RF receiver silent (no LED, no serial) | Power/wiring — check 5V VCC, GND, and that DATA (not NC) goes to GPIO 14; test with remote cm-close; check remote battery |
| ESP 3 can't reach cooler/light | ESP 1's DHCP IP changed — update `PEER_IP` in `esp3/config.h` |

## Repo layout

```text
esp1/             ESP 1 sketch (ESP8266) — flash this to ESP 1
esp3/             ESP 3 sketch (ESP32) — flash this to ESP 3
RelayBoard/       older copy of the ESP 1 sketch (superseded)
ServerBoard/      older copy of the ESP 3 sketch (superseded)
ButtonController/ legacy reference code
```
