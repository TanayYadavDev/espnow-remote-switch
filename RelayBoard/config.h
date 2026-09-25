#pragma once
// ===== RelayBoard settings — edit for your hardware =====

#define RELAY1_PIN   D5
#define RELAY2_PIN   D6
#define WIFI_CHANNEL 1    // must match the controller's channel

// NOTE: this relay module is ACTIVE-LOW (LOW = ON, HIGH = OFF).
// If your module is active-high, flip the logic in RelayBoard.ino.

// MAC address of ESP 2 (RFBoard) — the board allowed to command this one.
// Get it with tools/GetMACAddress and replace the values below.
uint8_t CONTROLLER_MAC[] = { 0x40, 0x91, 0x51, 0x45, 0x23, 0xDD };
