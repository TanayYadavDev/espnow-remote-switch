#pragma once
// ===== RFBoard (ESP 2) settings — edit for your hardware =====

#define RF_PIN       D7
#define WIFI_CHANNEL 1    // must match ESP 1 (RelayBoard)

// MAC address of ESP 1 (RelayBoard). Get it with tools/GetMACAddress.
uint8_t RELAY_BOARD_MAC[] = { 0x40, 0xF5, 0x20, 0x38, 0x9A, 0x94 };

// 433 MHz remote button codes (watch the serial monitor to learn yours).
// The ServerBoard transmits these SAME codes, so this board can't tell
// the physical remote apart from the web dashboard — that's intentional.
// NOTE: the strips-toggle code was missing in the original sketch —
// replace the placeholder below with the real code from your remote.
#define RF_CODE_STRIPS_TOGGLE 0x00000000  // TODO: put your remote's code here
#define RF_CODE_RELAY2_ON     8355010
#define RF_CODE_RELAY2_OFF    8355016
#define RF_CODE_RELAY1_ON     8355012
#define RF_CODE_RELAY1_OFF    8355020
