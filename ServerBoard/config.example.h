#pragma once
// ===== ServerBoard (ESP 3) settings — TEMPLATE =====
// Copy this file to "config.h" (same folder) and fill in your real values.
// config.h is git-ignored so your WiFi password never ends up on GitHub.

// --- Home WiFi (the webserver joins this network) ---
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// --- Pins ---
#define RF_TX_PIN  D2    // TODO: confirm — 433 MHz TRANSMITTER data pin
#define STRIP_PIN  D1    // TODO: confirm — strips relay pin
#define STRIP_ACTIVE_LOW false  // set true if your strip relay is active-low

// --- RF codes this board TRANSMITS (must match RFBoard's config.h) ---
// Same codes as the physical remote — ESP 2 can't tell them apart.
#define RF_BIT_LENGTH     24
#define RF_CODE_RELAY1_ON  8355012  // cooler ON  (TODO: confirm mapping)
#define RF_CODE_RELAY1_OFF 8355020  // cooler OFF (TODO: confirm mapping)
#define RF_CODE_RELAY2_ON  8355010  // light ON   (TODO: confirm mapping)
#define RF_CODE_RELAY2_OFF 8355016  // light OFF  (TODO: confirm mapping)
