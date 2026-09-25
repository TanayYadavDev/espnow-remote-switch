/*
 * ButtonController — ESP8266 + ESP-NOW
 *
 * A push-button controller (uses the built-in FLASH button on D3).
 * Each press toggles a relay on the RelayBoard and the board replies
 * with the status of both relays.
 *
 * Setup: edit config.h (peer MAC, relay number), flash, done.
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include "config.h"

// ---- ESP-NOW protocol (shared with RelayBoard / RFController) ----
enum PacketType : uint8_t {
  CMD_SET_RELAY = 1,
  STATUS_REPLY  = 2
};

struct Packet {
  uint8_t type;
  uint8_t relay;          // 1 or 2
  bool    state;          // desired state for CMD_SET_RELAY
  bool    relayState[2];  // board state for STATUS_REPLY
};
// -------------------------------------------------------------------

Packet packet;
bool lastButtonState = HIGH;
bool relayState = false;
unsigned long lastPress = 0;

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

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);
  esp_now_add_peer(RELAY_BOARD_MAC, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

  Serial.println("ButtonController ready");
}

void loop() {
  bool current = digitalRead(BUTTON_PIN);

  if (current == LOW &&
      lastButtonState == HIGH &&
      millis() - lastPress > DEBOUNCE_MS) {

    lastPress = millis();
    relayState = !relayState;

    packet.type  = CMD_SET_RELAY;
    packet.relay = BUTTON_RELAY;
    packet.state = relayState;

    esp_now_send(RELAY_BOARD_MAC, (uint8_t *)&packet, sizeof(packet));

    Serial.printf("Relay %d -> %s\n", BUTTON_RELAY, relayState ? "ON" : "OFF");
  }

  lastButtonState = current;
}
