#pragma once
// ===== RFController settings — edit for your hardware =====

#define RF_PIN       D7
#define WIFI_CHANNEL 1    // must match the peer boards' channel

// MAC addresses of the peer boards (get them with tools/GetMACAddress).
uint8_t RELAY_BOARD_MAC[] = { 0x40, 0xF5, 0x20, 0x38, 0x9A, 0x94 };  // ESP #2: RelayBoard
uint8_t LIGHT_BOARD_MAC[] = { 0xC8, 0xF0, 0x9E, 0xA3, 0x7F, 0x90 };  // ESP #3: RGB strip + webserver

// 433 MHz remote button codes (watch the serial monitor to learn yours).
// NOTE: the strips-toggle code was missing in the original sketch —
// replace the placeholder below with the real code from your remote.
#define RF_CODE_STRIPS_TOGGLE 0x00000000  // TODO: put your remote's code here
#define RF_CODE_RELAY2_ON     8355010
#define RF_CODE_RELAY2_OFF    8355016
#define RF_CODE_RELAY1_ON     8355012
#define RF_CODE_RELAY1_OFF    8355020
