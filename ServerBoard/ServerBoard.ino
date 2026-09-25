/*
 * ServerBoard — ESP 3 (ESP8266 webserver + strip lights + 433 MHz RF transmitter)
 *
 * What it does:
 *   1. Webserver — the "Home Control" dashboard (Cooler / Light / Strips).
 *   2. Strip lights — driven directly from STRIP_PIN (local relay).
 *   3. RF transmitter — for Cooler/Light it transmits the SAME 433 MHz codes
 *      as the physical remote, so ESP 2 (RFBoard) picks them up and forwards
 *      them over ESP-NOW to ESP 1 (RelayBoard). The dashboard and the remote
 *      are indistinguishable downstream — by design.
 *
 * State shown on the dashboard is tracked locally (optimistic): this board
 * assumes the command went through when it transmits the RF code.
 *
 * Setup: copy config.example.h to config.h, fill in WiFi + pins, flash, done.
 * Needs the RCSwitch library.
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <RCSwitch.h>
#include "config.h"

ESP8266WebServer server(80);
RCSwitch rf = RCSwitch();

// Dashboard state (optimistic — updated when we transmit).
bool coolerState = false;
bool lightState  = false;
bool stripsState = false;

// ---------- actions ----------

void applyStrips() {
  bool level = (stripsState != STRIP_ACTIVE_LOW) ? HIGH : LOW;
  digitalWrite(STRIP_PIN, level);
  Serial.printf("Strips -> %s\n", stripsState ? "ON" : "OFF");
}

// Transmit an RF code, like pressing the physical remote's button.
void transmitCode(unsigned long code, const char *label) {
  rf.send(code, RF_BIT_LENGTH);
  Serial.printf("RF TX %s (code %lu)\n", label, code);
}

void setCooler(bool state) {
  coolerState = state;
  transmitCode(state ? RF_CODE_RELAY1_ON : RF_CODE_RELAY1_OFF,
               state ? "cooler ON" : "cooler OFF");
}

void setLight(bool state) {
  lightState = state;
  transmitCode(state ? RF_CODE_RELAY2_ON : RF_CODE_RELAY2_OFF,
               state ? "light ON" : "light OFF");
}

void setStrips(bool state) {
  stripsState = state;
  applyStrips();
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
  <p class="foot">Ready<br>ServerBoard @ <span id="ip"></span></p>
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
  if (dev == "cooler")      setCooler(!coolerState);
  else if (dev == "light")  setLight(!lightState);
  else if (dev == "strips") setStrips(!stripsState);
  sendState();
}

// ---------- setup / loop ----------

void setup() {
  Serial.begin(115200);

  pinMode(STRIP_PIN, OUTPUT);
  setStrips(false);  // strips OFF on boot

  rf.enableTransmit(RF_TX_PIN);
  Serial.println("RF transmitter ready");

  WiFi.mode(WIFI_STA);

  // Uncomment for a fixed address (e.g. the old 192.168.0.189):
  // WiFi.config(IPAddress(192,168,0,189), IPAddress(192,168,0,1), IPAddress(255,255,255,0));

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
    Serial.println("WiFi failed — webserver offline");
  }

  server.on("/", handleRoot);
  server.on("/api/state", sendState);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("ServerBoard ready");
}

void loop() {
  server.handleClient();
}
