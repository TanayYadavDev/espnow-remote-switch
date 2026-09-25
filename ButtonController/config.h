#pragma once
// ===== ButtonController settings — edit for your hardware =====

#define BUTTON_PIN   D3    // built-in FLASH button
#define BUTTON_RELAY 1    // which relay on the RelayBoard this button toggles (1 or 2)
#define DEBOUNCE_MS  50
#define WIFI_CHANNEL 1    // must match RelayBoard's channel

// MAC address of the RelayBoard ESP8266.
// Get it with tools/GetMACAddress and replace the values below.
uint8_t RELAY_BOARD_MAC[] = { 0x40, 0xF5, 0x20, 0x38, 0x9A, 0x94 };
