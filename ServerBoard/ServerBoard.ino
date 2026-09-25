/*
 * ServerBoard — ESP 3 (ESP32 + WiFi + webserver + strips relay + 433 MHz RF receiver)
 *
 * New 2-board architecture (no ESP-NOW, no RF transmitter):
 *  - Drives the STRIP lights relay NATIVELY (boolean on/off).
 *  - Receives the 433 MHz physical remote:
 *      cooler/light buttons -> HTTP GET to ESP 1 (RelayBoard) API,
 *      strips button        -> toggles the local strips relay.
 *  - Hosts a web dashboard: strips toggled here, cooler/light proxied
 *    to ESP 1 over plain HTTP.
 *  - Exposes a tiny REST API so ESP 1 can command it:
 *      /api/strips/on | /api/strips/off | /api/strips/toggle | /api/strips/state
 *  - /api/state returns MERGED state: local strips + ESP 1's relay states
 *    (queried live), so both dashboards always show the truth.
 *
 * Setup: config.h already has WiFi (git-ignored). Check STRIP_PIN,
 * STRIP_ACTIVE_LOW, RF_RX_PIN and the RF codes, flash on ESP32
 * ("ESP32 Dev Module"), open http://192.168.0.189/
 */

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <RCSwitch.h>
#include "config.h"

WebServer server(80);
RCSwitch rf = RCSwitch();

bool stripsOn = false;

// ---- cached ESP 1 (RelayBoard) state ----
bool          peerCooler   = false;
bool          peerLight    = false;
bool          peerOnline   = false;
unsigned long lastPeerPoll = 0;
// -----------------------------------------

void applyStrips(bool on) {
  digitalWrite(STRIP_PIN, (on == STRIP_ACTIVE_LOW) ? LOW : HIGH);
}

void setStrips(bool on) {
  stripsOn = on;
  applyStrips(on);
  Serial.printf("Strips -> %s\n", on ? "ON" : "OFF");
}

// ---- HTTP helper (talks to ESP 1) ----
String peerURL(const String &path) {
  return String("http://") + PEER_IP + path;
}

// GET path on ESP 1. Returns true on HTTP 200, response body in `out`.
bool peerGET(const String &path, String &out) {
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(800);
  if (!http.begin(client, peerURL(path))) return false;
  int code = http.GET();
  bool ok = (code == 200);
  if (ok) out = http.getString();
  http.end();
  return ok;
}

void pollPeer() {
  if (millis() - lastPeerPoll < 1500) return;  // throttle peer queries
  lastPeerPoll = millis();
  String c, l;
  bool okC = peerGET("/api/cooler/state", c);
  bool okL = peerGET("/api/light/state", l);
  if (okC && okL) {
    peerOnline = true;
    peerCooler = (c.toInt() == 1);
    peerLight  = (l.toInt() == 1);
  } else {
    peerOnline = false;  // keep last-known values
  }
}

String stateJSON() {
  pollPeer();
  String j = "{";
  j += "\"cooler\":"      + String(peerCooler ? "true" : "false") + ",";
  j += "\"light\":"       + String(peerLight  ? "true" : "false") + ",";
  j += "\"strips\":"      + String(stripsOn   ? "true" : "false") + ",";
  j += "\"peer_online\":" + String(peerOnline ? "true" : "false");
  j += "}";
  return j;
}
// ---------------------------------------

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Home Control — ESP 3</title>
<style>
body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:16px}
h1{font-size:20px;margin:4px 0 12px}h1 small{color:#888;font-weight:normal}
.card{background:#1e1e1e;border-radius:12px;padding:16px;margin:10px 0;display:flex;
align-items:center;justify-content:space-between}
.card .name{font-size:18px}.card .st{font-size:13px;color:#aaa;margin-top:4px}
button{font-size:16px;padding:10px 22px;border:0;border-radius:8px;cursor:pointer}
.on{background:#2e7d32;color:#fff}.off{background:#444;color:#fff}
.peer{font-size:13px;color:#888;margin-top:14px}.dot{display:inline-block;width:9px;height:9px;
border-radius:50%;margin-right:6px;vertical-align:1px}
</style></head><body>
<h1>🏠 Home Control <small>— ESP 3 (strips)</small></h1>
<div class="card"><div><div class="name">❄️ Cooler</div><div class="st" id="s-cooler">…</div></div>
<button id="b-cooler" onclick="tg('cooler')">Toggle</button></div>
<div class="card"><div><div class="name">💡 Light</div><div class="st" id="s-light">…</div></div>
<button id="b-light" onclick="tg('light')">Toggle</button></div>
<div class="card"><div><div class="name">✨ Strips</div><div class="st" id="s-strips">…</div></div>
<button id="b-strips" onclick="tg('strips')">Toggle</button></div>
<div class="peer"><span class="dot" id="peer-dot" style="background:#888"></span>
<span id="peer-txt">ESP 1 (relays) …</span></div>
<script>
async function refresh(){
  try{
    const r=await fetch('/api/state');const s=await r.json();
    set('cooler',s.cooler);set('light',s.light);set('strips',s.strips);
    const d=document.getElementById('peer-dot'),t=document.getElementById('peer-txt');
    d.style.background=s.peer_online?'#2e7d32':'#c62828';
    t.textContent='ESP 1 (relays) '+(s.peer_online?'online':'OFFLINE');
  }catch(e){}
}
function set(dev,on){
  document.getElementById('s-'+dev).textContent=on?'ON':'OFF';
  const b=document.getElementById('b-'+dev);b.textContent=on?'Turn OFF':'Turn ON';
  b.className=on?'on':'off';
}
async function tg(dev){await fetch('/toggle?dev='+dev);refresh();}
refresh();setInterval(refresh,2000);
</script></body></html>
)rawliteral";

void handleRoot()   { server.send(200, "text/html", DASHBOARD_HTML); }
void handleState()  { server.send(200, "application/json", stateJSON()); }

void handleToggle() {
  String dev = server.arg("dev");
  String r;
  if (dev == "strips") {
    setStrips(!stripsOn);
  } else if (dev == "cooler") {
    peerGET("/api/cooler/toggle", r);   // proxied to ESP 1
    lastPeerPoll = 0;
  } else if (dev == "light") {
    peerGET("/api/light/toggle", r);    // proxied to ESP 1
    lastPeerPoll = 0;
  }
  server.send(200, "application/json", stateJSON());
}

// Native strips endpoints (used by ESP 1's dashboard proxy).
void handleNative(const String &action) {
  if (action == "on")          setStrips(true);
  else if (action == "off")    setStrips(false);
  else if (action == "toggle") setStrips(!stripsOn);
  else if (action == "state")  { server.send(200, "text/plain", stripsOn ? "1" : "0"); return; }
  server.send(200, "application/json", stateJSON());
}

// 433 MHz remote -> cooler/light via ESP 1's API, strips locally.
void handleRemoteCode(unsigned long code) {
  Serial.print("RF code: "); Serial.println(code);
  String r;
  switch (code) {
    case RF_CODE_RELAY1_ON:  peerGET("/api/cooler/on", r);  Serial.println("Cooler -> ON (via ESP 1)"); break;
    case RF_CODE_RELAY1_OFF: peerGET("/api/cooler/off", r); Serial.println("Cooler -> OFF (via ESP 1)"); break;
    case RF_CODE_RELAY2_ON:  peerGET("/api/light/on", r);   Serial.println("Light -> ON (via ESP 1)"); break;
    case RF_CODE_RELAY2_OFF: peerGET("/api/light/off", r);  Serial.println("Light -> OFF (via ESP 1)"); break;
    case RF_CODE_STRIPS_TOGGLE:
      if (code != 0) { setStrips(!stripsOn); lastPeerPoll = 0; }
      else Serial.println("Strips button pressed but RF_CODE_STRIPS_TOGGLE not set in config.h");
      break;
    default:
      Serial.println("Unknown code — add it to config.h if needed");
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(STRIP_PIN, OUTPUT);
  setStrips(false);

  rf.enableReceive(RF_RX_PIN);
  Serial.println("RF receiver ready");

  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(LOCAL_IP0, LOCAL_IP1, LOCAL_IP2, LOCAL_IP3),
              IPAddress(GW0, GW1, GW2, GW3),
              IPAddress(SN0, SN1, SN2, SN3));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi FAILED — check config.h (SSID/password).");
  } else {
    Serial.print("Dashboard: http://"); Serial.println(WiFi.localIP());
  }

  server.on("/",                 handleRoot);
  server.on("/api/state",        handleState);
  server.on("/toggle",           handleToggle);
  server.on("/api/strips/on",     [](){ handleNative("on"); });
  server.on("/api/strips/off",    [](){ handleNative("off"); });
  server.on("/api/strips/toggle", [](){ handleNative("toggle"); });
  server.on("/api/strips/state",  [](){ handleNative("state"); });
  server.begin();
  Serial.println("ServerBoard ready");
}

void loop() {
  server.handleClient();
  if (rf.available()) {
    handleRemoteCode(rf.getReceivedValue());
    rf.resetAvailable();
  }
}
