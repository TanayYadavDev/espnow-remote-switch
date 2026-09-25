#pragma once
// ===== ServerBoard (ESP 3, ESP32) settings — TEMPLATE =====
// Copy this file to "config.h" (same folder) and fill in your real values.
// config.h is git-ignored so your WiFi password never ends up on GitHub.

// --- Home WiFi ---
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASS "your-wifi-password"

// --- Static IP for THIS board (ESP 3) ---
#define LOCAL_IP0 192
#define LOCAL_IP1 168
#define LOCAL_IP2 0
#define LOCAL_IP3 189
#define GW0 192
#define GW1 168
#define GW2 0
#define GW3 1
#define SN0 255
#define SN1 255
#define SN2 255
#define SN3 0

// --- Peer board (ESP 1 / RelayBoard, cooler+light) ---
#define PEER_IP "192.168.0.145"

// --- Pins (ESP32 GPIO numbers, NOT D1/D2 labels) ---
#define STRIP_PIN        27    // TODO: confirm — strips relay pin (GPIO)
#define STRIP_ACTIVE_LOW true // TODO: confirm — true if relay is active-low (LOW = ON)
#define RF_RX_PIN        4     // TODO: confirm — 433 MHz RECEIVER data pin (GPIO)

// --- 433 MHz remote button codes (watch the serial monitor to learn yours) ---
#define RF_BIT_LENGTH       24
#define RF_CODE_RELAY1_ON   8355012  // cooler ON  (TODO: confirm mapping)
#define RF_CODE_RELAY1_OFF  8355020  // cooler OFF (TODO: confirm mapping)
#define RF_CODE_RELAY2_ON   8355010  // light ON   (TODO: confirm mapping)
#define RF_CODE_RELAY2_OFF  8355016  // light OFF  (TODO: confirm mapping)
// NOTE: the strips-toggle code was missing in the original sketch —
// press the remote's strips button, read "RF code: <number>" from the
// serial monitor, and put it here.
#define RF_CODE_STRIPS_TOGGLE 0x00000000  // TODO: put your remote's strips code here
