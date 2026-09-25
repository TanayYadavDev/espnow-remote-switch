#pragma once
// ===== RelayBoard (ESP 1, ESP8266) settings — TEMPLATE =====
// Copy to config.h and fill in your values. config.h is git-ignored.

// --- Home WiFi ---
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASS "your-wifi-password"

// --- Static IP for THIS board (ESP 1) ---
#define LOCAL_IP0 192
#define LOCAL_IP1 168
#define LOCAL_IP2 0
#define LOCAL_IP3 145
#define GW0 192
#define GW1 168
#define GW2 0
#define GW3 1
#define SN0 255
#define SN1 255
#define SN2 255
#define SN3 0

// --- Peer board (ESP 3 / ServerBoard, strips) ---
#define PEER_IP "192.168.0.189"

// --- Relay wiring ---
#define RELAY1_PIN D5
#define RELAY2_PIN D6
#define RELAY_ACTIVE_LOW true   // set false if your relay module is active-high

// TODO: confirm which relay drives which appliance
#define COOLER_PIN RELAY1_PIN
#define LIGHT_PIN  RELAY2_PIN
