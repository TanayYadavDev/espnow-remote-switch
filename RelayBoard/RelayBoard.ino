/*
 * RelayBoard — ESP8266 + ESP-NOW
 *
 * The "muscle" of the system: a 2-channel relay board that listens for
 * ESP-NOW commands and switches the relays. After every command it sends
 * back a STATUS_REPLY with the current state of both relays.
 *
 * The relays on this board are ACTIVE-LOW: pin LOW = relay ON.
 * Both relays are forced OFF (pin HIGH) at boot.
 *
 * Setup: edit config.h (controller MAC, relay pins), flash, done.
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include "config.h"

// ---- ESP-NOW protocol (shared with the controller nodes) ----
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
// --------------------------------------------------------------

Packet packet;

void sendStatus() {
  packet.type = STATUS_REPLY;

  // Relays are active-low: LOW pin means ON.
  packet.relayState[0] = (digitalRead(RELAY1_PIN) == LOW);
  packet.relayState[1] = (digitalRead(RELAY2_PIN) == LOW);

  esp_now_send(CONTROLLER_MAC, (uint8_t *)&packet, sizeof(packet));
}

void onReceive(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len < sizeof(Packet)) return;
  memcpy(&packet, incomingData, sizeof(packet));

  if (packet.type == CMD_SET_RELAY) {
    uint8_t pin;
    if (packet.relay == 1)      pin = RELAY1_PIN;
    else if (packet.relay == 2) pin = RELAY2_PIN;
    else return;  // unknown relay number — ignore

    digitalWrite(pin, packet.state ? LOW : HIGH);  // active-low
    sendStatus();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Relays OFF on boot (active-low boards: HIGH = OFF).
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onReceive);
  esp_now_add_peer(CONTROLLER_MAC, ESP_NOW_ROLE_CONTROLLER, WIFI_CHANNEL, NULL, 0);

  Serial.println("RelayBoard ready");
}

void loop() {
  // Everything happens in the ESP-NOW receive callback.
}
