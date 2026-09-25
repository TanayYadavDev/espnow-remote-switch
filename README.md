# ESP-NOW Remote Switch

Wireless home-appliance control with 3 ESP8266 boards: ESP-NOW between the
relay board and the RF board, 433 MHz RF between the server board and the RF
board, and a WiFi webserver for phone control.

## Architecture — 3 ESPs

```
  433 MHz remote ──┐
                   ▼
            ┌─────────────┐   ESP-NOW    ┌──────────────┐
            │ ESP 2       │◄────────────►│ ESP 1        │
            │ RFBoard     │  CMD_SET_RELAY│ RelayBoard   │
            │ (the brain) │  STATUS_REPLY │ (the muscle) │
            └──────┬──────┘              └──────┬───────┘
                   ▲ 433 MHz RF                │ 2-channel relays
                   │ (same codes               │ (active-low)
                   │  as remote!)              ▼
            ┌──────┴──────┐              Cooler / Light
            │ ESP 3       │
            │ ServerBoard │
            │ webserver   │──▶ phone dashboard ("Home Control")
            │ strips      │──▶ local relay on STRIP_PIN
            └─────────────┘
```

How a web toggle flows: phone → ESP 3 webserver → 433 MHz RF transmit
(same code as the remote's button) → ESP 2 receives → ESP-NOW → ESP 1
switches the relay → ESP 1 replies STATUS_REPLY → ESP 2 prints it.

## Nodes

| Folder | ESP | Role |
|---|---|---|
| `RelayBoard/` | ESP 1 | 2-channel relay board (active-low). Receives ESP-NOW commands from ESP 2, replies with STATUS_REPLY |
| `RFBoard/` | ESP 2 | 433 MHz RF receiver + ESP-NOW controller. Remote (or ESP 3) → ESP-NOW commands to ESP 1 |
| `ServerBoard/` | ESP 3 | WiFi webserver ("Home Control" dashboard) + strip lights on STRIP_PIN + 433 MHz RF transmitter toward ESP 2 |
| `ButtonController/` | (optional) | Toggles a relay with the built-in FLASH button (D3), talks ESP-NOW to ESP 1 |
| `tools/GetMACAddress/` | — | Prints a board's MAC address — needed for pairing |

## Hardware

- 3× ESP8266 boards (e.g. NodeMCU / Wemos D1 mini)
- 1× 2-channel relay module (**active-low**) for ESP 1
- 1× relay (or MOSFET) module for the strip lights on ESP 3
- 1× 433 MHz RF receiver module + RF remote (ESP 2)
- 1× 433 MHz RF transmitter module (ESP 3)

## Setup

1. **Get MAC addresses**: flash `tools/GetMACAddress` on ESP 1 and ESP 2,
   open the serial monitor (115200 baud), note both MACs.
2. **Edit configs**:
   - `RFBoard/config.h` — put ESP 1's MAC in `RELAY_BOARD_MAC`; check the RF codes.
   - `RelayBoard/config.h` — put ESP 2's MAC in `CONTROLLER_MAC`.
   - `ServerBoard/config.h` — copy from `config.example.h`; fill `WIFI_SSID`/`WIFI_PASS`
     (git-ignored — your password never leaves your machine), `RF_TX_PIN`,
     `STRIP_PIN`, and confirm the RF code mapping.
3. **Flash**: `RelayBoard` on ESP 1, `RFBoard` on ESP 2, `ServerBoard` on ESP 3.
4. Open the IP printed by ESP 3's serial monitor — the **Home Control** dashboard
   appears (Cooler / Light / Strips cards with ON/OFF buttons, auto-refreshing).

`RFBoard` and `ServerBoard` need the [RCSwitch](https://github.com/sui77/rc-switch) library.

## Protocol

**ESP-NOW** (ESP 2 ↔ ESP 1) — one tiny packet:

```c
enum PacketType : uint8_t { CMD_SET_RELAY = 1, STATUS_REPLY = 2 };

struct Packet {
  uint8_t type;
  uint8_t relay;          // 1 or 2
  bool    state;          // desired state
  bool    relayState[2];  // board state (STATUS_REPLY)
};
```

**433 MHz RF** (remote → ESP 2, ESP 3 → ESP 2) — plain remote codes:

| Code | Action |
|---|---|
| 8355012 / 8355020 | Relay 1 ON / OFF (Cooler) |
| 8355010 / 8355016 | Relay 2 ON / OFF (Light) |
| _(unknown)_ | Strips toggle (code missing — see TODO in `RFBoard/config.h`) |

ESP 3 transmits the same codes as the physical remote, so ESP 2 treats the
web dashboard and the remote identically.

## Troubleshooting

- **Commands not arriving** → MACs in `config.h` correct? `WIFI_CHANNEL` matches on ESP 1 + ESP 2?
- **Dashboard IP changed** → ESP 3 gets its IP via DHCP; check its serial monitor on boot
  (or uncomment the static-IP block in `ServerBoard.ino`).
- **Relay does the opposite** → relay module is probably active-high; flip the
  `LOW`/`HIGH` logic in `RelayBoard.ino` (or `STRIP_ACTIVE_LOW` for the strips).
- **RF remote does nothing** → open ESP 2's serial monitor: unknown codes get
  printed with a hint. Add them to `RFBoard/config.h`.
- **Web toggle does nothing** → open ESP 2's serial monitor while toggling: if no
  RF code appears, ESP 3's transmitter (`RF_TX_PIN`) isn't reaching ESP 2.
