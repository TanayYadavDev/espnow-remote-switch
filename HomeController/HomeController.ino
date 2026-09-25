/*
 * HomeController — ESP8266 (merged node)
 *
 * Replaces the old RFController + LightBoard (ESP #3) pair with ONE board.
 * This ESP does everything:
 *   1. Webserver  — "Home Control" dashboard (Cooler / Light / Strips toggles)
 *   2. Strip lights — driven directly from STRIP_PIN (local relay)
 *   3. ESP-NOW sender — Cooler/Light commands go to the RelayBoard over ESP-NOW
 *   4. 433 MHz RF remote receiver — same remote codes as before
 *
 * The RelayBoard (other ESP) is unchanged: it receives CMD_SET_RELAY and
 * replies with STATUS_REPLY, which keeps this dashboard in sync even when
 * the RF remote is used.
 *
 * Setup: edit config.h (WiFi credentials, pins, MAC), flash, done.
 * NOTE: ESP-NOW and the webserver share the WiFi radio, so WIFI_CHANNEL
 * should match your router's channel (check the boot log: it prints it).
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <espnow.h>
#include <RCSwitch.h>
#include "config.h"

// ---- ESP-NOW protocol (shared with RelayBoard) ----
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
// ----------------------------------------------------

ESP8266WebServer server(80);
RCSwitch rf = RCSwitch();
Packet packet;

// Single source of truth for the dashboard.
bool coolerState = false;
bool lightState  = false;
bool stripsState = false;

// ---------- helpers ----------

bool relayStateOf(uint8_t relay) {
  return (relay == RELAY_COOLER) ? coolerState : lightState;
}

void applyStrips() {
  bool level = (stripsState != STRIP_ACTIVE_LOW) ? HIGH : LOW;
  digitalWrite(STRIP_PIN, level);
  Serial.printf("Strips -> %s\n", stripsState ? "ON" : "OFF");
}

void setStrips(bool state) {
  stripsState = state;
  applyStrips();
}

// Send a relay command to the RelayBoard over ESP-NOW.
void sendRelayCommand(uint8_t relay, bool state) {
  if (relay == RELAY_COOLER)      coolerState = state;
  else if (relay == RELAY_LIGHT)  lightState  = state;
  else return;

  packet.type  = CMD_SET_RELAY;
  packet.relay = relay;
  packet.state = state;

  esp_now_send(RELAY_BOARD_MAC, (uint8_t *)&packet, sizeof(packet));
  Serial.printf("RelayBoard Relay %d -> %s\n", relay, state ? "ON" : "OFF");
}

// ---------- ESP-NOW callbacks ----------

void onSent(uint8_t *mac, uint8_t status) {
  Serial.print("ESP-NOW send: ");
  Serial.println(status == 0 ? "OK" : "FAILED");
}

void onReceive(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len < sizeof(Packet)) return;
  memcpy(&packet, incomingData, sizeof(packet));

  // RelayBoard reports back after every command — adopt it as truth
  // so the dashboard stays correct when the RF remote is used.
  if (packet.type == STATUS_REPLY) {
    coolerState = packet.relayState[RELAY_COOLER - 1];
    lightState  = packet.relayState[RELAY_LIGHT - 1];
    Serial.printf("Status: Cooler=%s Light=%s\n",
                  coolerState ? "ON" : "OFF", lightState ? "ON" : "OFF");
  }
}

// ---------- webserver ----------

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Home Control</title>
<style>
  body{background:#0b0d12;color:#fff;font-family:sans-serif;margin:0;
       display:flex;justify-content:center;align-items:center;min-height:100vh;}
  .card{background:#151923;border-radius:24px;padding:32px;width:340px;max-width:90vw;}
  h1{margin:0 0 4px;font-size:28px;}
  .sub{color:#8a93a6;margin:0 0 24px;}
  .row{background:#1d2230;border-radius:16px;padding:18px 20px;margin-bottom:14px;
       display:flex;justify-content:space-between;align-items:center;}
  .row b{font-size:20px;display:block;}
  .st{color:#8a93a6;font-size:14px;}
  .st.on{color:#4ade80;}
  button{background:#2a3145;color:#fff;border:none;border-radius:12px;
         padding:14px 20px;font-size:15px;font-weight:bold;cursor:pointer;}
  button:active{background:#3a435c;}
  .foot{text-align:center;color:#8a93a6;margin-top:20px;font-size:14px;}
</style>
</head>
<body>
<div class="card">
  <h1>Home Control</h1>
  <p class="sub">Local LAN Controller</p>
  <div class="row">
    <div><b>Cooler</b><span class="st" id="s-cooler">OFF</span></div>
    <button onclick="tgl('cooler')">ON / OFF</button>
  </div>
  <div class="row">
    <div><b>Light</b><span class="st" id="s-light">OFF</span></div>
    <button onclick="tgl('light')">ON / OFF</button>
  </div>
  <div class="row">
    <div><b>Strips</b><span class="st" id="s-strips">OFF</span></div>
    <button onclick="tgl('strips')">ON / OFF</button>
  </div>
  <p class="foot">Ready<br>HomeController @ <span id="ip"></span></p>
</div>
<script>
function setState(id,on){
  var el=document.getElementById(id);
  el.textContent=on?'ON':'OFF';
  el.className='st'+(on?' on':'');
}
async function refresh(){
  try{
    var r=await fetch('/api/state');
    var j=await r.json();
    setState('s-cooler',j.cooler);
    setState('s-light',j.light);
    setState('s-strips',j.strips);
    if(j.ip)document.getElementById('ip').textContent=j.ip;
  }catch(e){}
}
async function tgl(dev){
  try{await fetch('/toggle?dev='+dev);}catch(e){}
  refresh();
}
setInterval(refresh,3000);
refresh();
</script>
</body>
</html>
)rawliteral";

void sendState() {
  String json = "{\"cooler\":";
  json += coolerState ? "true" : "false";
  json += ",\"light\":";
  json += lightState ? "true" : "false";
  json += ",\"strips\":";
  json += stripsState ? "true" : "false";
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

void handleToggle() {
  String dev = server.arg("dev");
  if (dev == "cooler")      sendRelayCommand(RELAY_COOLER, !coolerState);
  else if (dev == "light")  sendRelayCommand(RELAY_LIGHT, !lightState);
  else if (dev == "strips") setStrips(!stripsState);
  sendState();
}

// ---------- RF remote ----------

void handleRfCode(unsigned long code) {
  Serial.print("RF code: ");
  Serial.println(code);

  switch (code) {
    case RF_CODE_STRIPS_TOGGLE:
      setStrips(!stripsState);
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
}

// ---------- setup / loop ----------

void setup() {
  Serial.begin(115200);

  pinMode(STRIP_PIN, OUTPUT);
  setStrips(false);  // strips OFF on boot

  rf.enableReceive(digitalPinToInterrupt(RF_PIN));
  Serial.println("RF receiver ready");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Webserver at http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed — webserver offline, ESP-NOW/RF still work");
  }
  Serial.printf("WiFi channel: %d (RelayBoard must use the same)\n", WiFi.channel());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);
  esp_now_add_peer(RELAY_BOARD_MAC, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

  server.on("/", handleRoot);
  server.on("/api/state", sendState);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("HomeController ready");
}

void loop() {
  server.handleClient();

  if (rf.available()) {
    handleRfCode(rf.getReceivedValue());
    rf.resetAvailable();
  }
}
