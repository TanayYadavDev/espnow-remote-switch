# ESP-NOW Remote Switch

Wireless home-appliance control with ESP8266 boards talking over **ESP-NOW** (no WiFi router needed for the link).

## Architecture — 2 ESPs

```
                    ┌─────────────────────┐
                    │    HomeController   │  (ESP #1 — everything on one board)
                    │                     │
  433 MHz remote ──▶│  RF receiver        │
                    │  Webserver (:80)     │──▶ "Home Control" dashboard
                    │  Strip lights       │──▶ local relay on STRIP_PIN
                    │  ESP-NOW sender     │──▶┐
                    └─────────────────────┘  │ ESP-NOW
                                             ▼
                    ┌─────────────────────┐
                    │     RelayBoard      │  (ESP #2 — the muscle)
                    │  2-channel relays   │──▶ relay 1 = Cooler, relay 2 = Light
                    │  (active-low)       │──▶ replies STATUS_REPLY after every command
                    └─────────────────────┘
```

The old 3-ESP setup (separate RFController + LightBoard) is replaced by the single
**HomeController**. `ButtonController` still works as an optional extra remote.

## Nodes

| Folder | Board | Role |
|---|---|---|
| `HomeController/` | ESP8266 | **(recommended)** RF receiver + webserver + strip lights + ESP-NOW sender, all in one |
| `RelayBoard/` | ESP8266 | 2-channel relay board (active-low), replies with status after every command |
| `ButtonController/` | ESP8266 | (optional) Toggles a relay with the built-in FLASH button (D3) |
| `RFController/` | ESP8266 | (legacy) Standalone RF→ESP-NOW bridge without webserver — superseded by HomeController |
| `tools/GetMACAddress/` | ESP8266/ESP32 | Prints the board's MAC address — needed for pairing |

## Hardware

- 3× ESP8266 boards (e.g. NodeMCU / Wemos D1 mini)
- 1× 2-channel relay module (**active-low** — adjust logic in `RelayBoard.ino` if yours is active-high)
- 1× 433 MHz RF receiver module + RF remote (for `RFController`)

## Setup

1. **Get the RelayBoard's MAC address**: flash `tools/GetMACAddress` on it, open the serial monitor (115200 baud), note the MAC.
2. **Edit `HomeController/config.h`** (copy from `config.example.h` if it doesn't exist):
   - `WIFI_SSID` / `WIFI_PASS` — your home WiFi (the dashboard joins this network).
     This file is git-ignored — your password never leaves your machine,
   - paste the RelayBoard's MAC into `RELAY_BOARD_MAC`,
   - set `STRIP_PIN` to the pin your strip relay is on,
   - map `RELAY_COOLER` / `RELAY_LIGHT` to the correct relay numbers,
   - add your RF remote's button codes (watch the serial monitor while pressing buttons).
3. **Flash**: `HomeController` on one ESP, `RelayBoard` (edit its `config.h` first) on the other.
4. Open the IP printed on the serial monitor — the **Home Control** dashboard appears
   (same UI as before: Cooler / Light / Strips cards with ON/OFF buttons, auto-refreshing).

`RFController` and `ButtonController` need the [RCSwitch](https://github.com/sui77/rc-switch) library.

## Protocol

All nodes share a tiny packet over ESP-NOW:

```c
enum PacketType : uint8_t { CMD_SET_RELAY = 1, STATUS_REPLY = 2, CMD_SET_STRIPS = 3 };

struct Packet {
  uint8_t type;
  uint8_t relay;          // 1 or 2
  bool    state;          // desired state
  bool    relayState[2];  // board state (STATUS_REPLY)
};
```

- Controller → board: `CMD_SET_RELAY` (or `CMD_SET_STRIPS` for the light board).
- Board → controller: `STATUS_REPLY` with the live state of both relays.

## Troubleshooting

- **Commands not arriving** → check that the MACs in `config.h` are correct and that `WIFI_CHANNEL` matches your router's WiFi channel (the sketch prints the actual channel on boot — ESP-NOW and the webserver share the radio, so they must agree).
- **Dashboard IP changed** → the ESP gets its IP from your router via DHCP; check the serial monitor on boot, or set a static lease in your router.
- **Relay does the opposite** → your relay module is probably active-high; flip the `LOW`/`HIGH` logic in `RelayBoard.ino` (or `STRIP_ACTIVE_LOW` for the strips).
- **RF remote does nothing** → open the serial monitor: unknown codes get printed with a hint. Add them to `config.h`.
