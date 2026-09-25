/*
 * RFController — ESP8266 + ESP-NOW + 433 MHz RF remote
 *
 * Listens for button presses from a 433 MHz RF remote (RCSwitch on RF_PIN)
 * and forwards them over ESP-NOW:
 *   - ESP #2 (RelayBoard) : switchboard / cooler / light relays
 *   - ESP #3 (LightBoard) : RGB strip relay + webserver (CMD_SET_STRIPS)
 *
 * Setup: edit config.h (peer MACs, RF codes), flash, done.
 * New remote? Watch the serial monitor, note the code, add it to config.h.
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <RCSwitch.h>
#include "config.h"

// ---- ESP-NOW protocol (shared with the other nodes) ----
enum PacketType : uint8_t {
  CMD_SET_RELAY  = 1,
  STATUS_REPLY   = 2,
  CMD_SET_STRIPS = 3
};

struct Packet {
  uint8_t type;
  uint8_t relay;          // 1 or 2
  bool    state;          // desired state
  bool    relayState[2];  // board state for STATUS_REPLY
};
// ---------------------------------------------------------

RCSwitch rf = RCSwitch();
Packet packet;

bool relayState[3] = { false, false, false };  // index 1..2 used
bool stripsState = false;

void onSent(uint8_t *mac, uint8_t status) {
  Serial.print("Send: ");
  Serial.println(status == 0 ? "OK" : "FAILED");
}

void onReceive(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len < sizeof(Packet)) return;
  memcpy(&packet, incomingData, sizeof(packet));

  if (packet.type == STATUS_REPLY) {
    Serial.println("Relay Status:");
    for (int i = 0; i < 2; i++) {
      Serial.printf("  Relay %d : %s\n", i + 1, packet.relayState[i] ? "ON" : "OFF");
    }
    Serial.println();
  }
}

// Send a relay command to ESP #2 (RelayBoard)
void sendRelayCommand(uint8_t relay, bool state) {
  if (relay < 1 || relay > 2) return;

  relayState[relay] = state;

  packet.type  = CMD_SET_RELAY;
  packet.relay = relay;
  packet.state = state;

  esp_now_send(RELAY_BOARD_MAC, (uint8_t *)&packet, sizeof(packet));
  Serial.printf("RelayBoard Relay %d -> %s\n", relay, state ? "ON" : "OFF");
}

// Send a strips command to ESP #3 (LightBoard)
void sendStripsCommand() {
  packet.type  = CMD_SET_STRIPS;
  packet.relay = 0;
  packet.state = stripsState;

  esp_now_send(LIGHT_BOARD_MAC, (uint8_t *)&packet, sizeof(packet));
  Serial.printf("LightBoard strips -> %s\n", stripsState ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  rf.enableReceive(digitalPinToInterrupt(RF_PIN));
  Serial.println("RF receiver ready");

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);

  esp_now_add_peer(RELAY_BOARD_MAC, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);
  esp_now_add_peer(LIGHT_BOARD_MAC, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

  Serial.println("RFController ready — listening for 433 MHz remote");
}

void loop() {
  if (rf.available()) {
    unsigned long code = rf.getReceivedValue();

    Serial.print("RF code: ");
    Serial.println(code);

    switch (code) {
      case RF_CODE_STRIPS_TOGGLE:
        stripsState = !stripsState;
        sendStripsCommand();
        delay(250);
        break;

      case RF_CODE_RELAY2_ON:
        sendRelayCommand(2, true);
        break;

      case RF_CODE_RELAY2_OFF:
        sendRelayCommand(2, false);
        break;

      case RF_CODE_RELAY1_ON:
        sendRelayCommand(1, true);
        break;

      case RF_CODE_RELAY1_OFF:
        sendRelayCommand(1, false);
        break;

      default:
        Serial.println("Unknown code — add it to config.h if needed");
        break;
    }

    rf.resetAvailable();
  }
}
