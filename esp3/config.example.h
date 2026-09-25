#pragma once
// ===== esp3 (ESP 3, ESP32) settings — TEMPLATE =====
// Copy this file to config.h and fill in your values.
// config.h is git-ignored: real credentials never get committed.

// --- Home WiFi ---
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASS "YourWiFiPassword"

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

// --- Peer board (ESP 1, cooler+light) ---
// NOTE: ESP 1 is on DHCP — check its Serial Monitor for the current IP
// and keep this updated, or cooler/light control from ESP 3 will break.
#define PEER_IP "192.168.0.132"

// --- Pins (ESP32 GPIO numbers, NOT D1/D2 labels) ---
#define STRIP_PIN        12    // strips relay (GPIO 12)
#define STRIP_ACTIVE_LOW false // relay is ACTIVE-HIGH (HIGH = ON)
#define RF_RX_PIN        14    // 433 MHz RECEIVER data pin (GPIO 14)

// --- 433 MHz remote button codes (watch the serial monitor to learn yours) ---
#define RF_BIT_LENGTH       24
#define RF_CODE_RELAY1_ON    8355012  // cooler ON
#define RF_CODE_RELAY1_OFF  8355020  // cooler OFF
#define RF_CODE_RELAY2_ON    8355010  // light ON
#define RF_CODE_RELAY2_OFF  8355016  // light OFF
#define RF_CODE_STRIPS_TOGGLE 8355009  // strips toggle
// NOTE: press any remote button and read "RF code: <number>" from the
// serial monitor to learn codes for a new remote.
