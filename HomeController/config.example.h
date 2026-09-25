#pragma once
// ===== HomeController settings — TEMPLATE =====
// Copy this file to "config.h" (same folder) and fill in your real values.
// config.h is git-ignored so your WiFi password never ends up on GitHub.

// --- Home WiFi (the webserver joins this network) ---
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// --- ESP-NOW channel: should match your router's WiFi channel ---
// (the sketch prints the actual channel on boot — set this to match)
#define WIFI_CHANNEL 1

// --- Pins ---
#define RF_PIN    D7     // 433 MHz receiver module
#define STRIP_PIN D1     // strips relay pin
#define STRIP_ACTIVE_LOW false  // set true if your strip relay is active-low

// --- Peer: the RelayBoard's MAC (get it with tools/GetMACAddress) ---
uint8_t RELAY_BOARD_MAC[] = { 0x40, 0xF5, 0x20, 0x38, 0x9A, 0x94 };

// --- Which RelayBoard relay is which (1 or 2) ---
// The dashboard shows "Cooler" and "Light" — map them to the right relays.
#define RELAY_COOLER 1
#define RELAY_LIGHT  2

// --- 433 MHz remote button codes (watch the serial monitor to learn yours) ---
// Strips code unknown? Flash the sketch, press the remote's strips button —
// the serial monitor prints "RF code: <number>"; put that number below.
#define RF_CODE_STRIPS_TOGGLE 0x00000000  // TODO: put your remote's code here
#define RF_CODE_RELAY2_ON     8355010
#define RF_CODE_RELAY2_OFF    8355016
#define RF_CODE_RELAY1_ON     8355012
#define RF_CODE_RELAY1_OFF    8355020
