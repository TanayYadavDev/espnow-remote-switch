#pragma once
// ===== esp1 (ESP 1, ESP8266) settings — TEMPLATE =====
// Copy this file to config.h and fill in your values.
// config.h is git-ignored: real credentials never get committed.

// --- Home WiFi ---
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASS "YourWiFiPassword"

// --- IP: DHCP (router assigns it; check Serial Monitor for the dashboard URL) ---

// --- Peer board (ESP 3, strips) — static IP ---
#define PEER_IP "192.168.0.189"

// --- Relay wiring (active-low module: LOW = ON) ---
#define RELAY1_PIN D5
#define RELAY2_PIN D6
#define RELAY_ACTIVE_LOW true

// TODO: confirm which relay drives which appliance
#define COOLER_PIN RELAY1_PIN
#define LIGHT_PIN  RELAY2_PIN
