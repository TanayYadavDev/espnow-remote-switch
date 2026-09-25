/*
 * RelayBoard — ESP 1 (ESP8266 + WiFi + webserver + 2 relays)
 *
 * New 2-board architecture (no ESP-NOW):
 *  - Drives the cooler + light relays NATIVELY (D5/D6, active-low).
 *  - Hosts a web dashboard: cooler/light toggled here, strips proxied
 *    to ESP 3 (ServerBoard) over plain HTTP.
 *  - Exposes a tiny REST API so ESP 3 can command it:
 *      /api/cooler/on | /api/cooler/off | /api/cooler/toggle | /api/cooler/state
 *      /api/light/on  | /api/light/off  | /api/light/toggle  | /api/light/state
 *    (ESP 3's 433 MHz remote handler uses these.)
 *  - /api/state returns MERGED state: local relays + ESP 3's strips state
 *    (queried live), so both dashboards always show the truth.
 *
 * Setup: copy config.example.h -> config.h, fill WiFi + check pins,
 * flash on ESP8266 ("NodeMCU 1.0"), open http://192.168.0.145/
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "config.h"

ESP8266WebServer server(80);

bool coolerOn = false;
bool lightOn  = false;

// ---- cached ESP 3 (ServerBoard / strips) state ----
bool          peerStrips   = false;
bool          peerOnline   = false;
unsigned long lastPeerPoll = 0;
// ---------------------------------------------------

void applyRelay(uint8_t pin, bool on) {
  digitalWrite(pin, (on == RELAY_ACTIVE_LOW) ? LOW : HIGH);
}

void setCooler(bool on) { coolerOn = on; applyRelay(COOLER_PIN, on); }
void setLight(bool on)  { lightOn  = on; applyRelay(LIGHT_PIN,  on); }

// ---- HTTP helper (talks to ESP 3) ----
String peerURL(const String &path) {
  return String("http://") + PEER_IP + path;
}

// GET path on ESP 3. Returns true on HTTP 200, response body in `out`.
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
  String s;
  if (peerGET("/api/strips/state", s)) {
    peerOnline = true;
    peerStrips = (s.toInt() == 1);
  } else {
    peerOnline = false;  // keep last-known peerStrips
  }
}

String stateJSON() {
  pollPeer();
  String j = "{";
  j += "\"cooler\":"      + String(coolerOn  ? "true" : "false") + ",";
  j += "\"light\":"       + String(lightOn   ? "true" : "false") + ",";
  j += "\"strips\":"      + String(peerStrips ? "true" : "false") + ",";
  j += "\"peer_online\":" + String(peerOnline ? "true" : "false");
  j += "}";
  return j;
}
// ---------------------------------------

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Home Control — ESP 1</title>
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
<h1>🏠 Home Control <small>— ESP 1 (relays)</small></h1>
<div class="card"><div><div class="name">❄️ Cooler</div><div class="st" id="s-cooler">…</div></div>
<button id="b-cooler" onclick="tg('cooler')">Toggle</button></div>
<div class="card"><div><div class="name">💡 Light</div><div class="st" id="s-light">…</div></div>
<button id="b-light" onclick="tg('light')">Toggle</button></div>
<div class="card"><div><div class="name">✨ Strips</div><div class="st" id="s-strips">…</div></div>
<button id="b-strips" onclick="tg('strips')">Toggle</button></div>
<div class="peer"><span class="dot" id="peer-dot" style="background:#888"></span>
<span id="peer-txt">ESP 3 (strips) …</span></div>
<script>
async function refresh(){
  try{
    const r=await fetch('/api/state');const s=await r.json();
    set('cooler',s.cooler);set('light',s.light);set('strips',s.strips);
    const d=document.getElementById('peer-dot'),t=document.getElementById('peer-txt');
    d.style.background=s.peer_online?'#2e7d32':'#c62828';
    t.textContent='ESP 3 (strips) '+(s.peer_online?'online':'OFFLINE');
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
  if (dev == "cooler")      setCooler(!coolerOn);
  else if (dev == "light")  setLight(!lightOn);
  else if (dev == "strips") {
    String r;
    peerGET("/api/strips/toggle", r);  // proxied to ESP 3
    lastPeerPoll = 0;                  // force fresh peer state below
  }
  server.send(200, "application/json", stateJSON());
}

// Native on/off/toggle/state endpoints (used by ESP 3's remote handler).
void handleNative(const String &dev, const String &action) {
  bool *flag = (dev == "cooler") ? &coolerOn : &lightOn;
  if (action == "on")          { if (dev == "cooler") setCooler(true);  else setLight(true); }
  else if (action == "off")    { if (dev == "cooler") setCooler(false); else setLight(false); }
  else if (action == "toggle") { if (dev == "cooler") setCooler(!coolerOn); else setLight(!lightOn); }
  else if (action == "state")  { server.send(200, "text/plain", *flag ? "1" : "0"); return; }
  server.send(200, "application/json", stateJSON());
}

void setup() {
  Serial.begin(115200);

  pinMode(COOLER_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  setCooler(false);
  setLight(false);

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
    Serial.println("WiFi FAILED — check config.h (SSID/password). Relays won't respond to HTTP until WiFi is up.");
  } else {
    Serial.print("Dashboard: http://"); Serial.println(WiFi.localIP());
  }

  server.on("/",                 handleRoot);
  server.on("/api/state",        handleState);
  server.on("/toggle",           handleToggle);
  server.on("/api/cooler/on",     [](){ handleNative("cooler","on"); });
  server.on("/api/cooler/off",    [](){ handleNative("cooler","off"); });
  server.on("/api/cooler/toggle", [](){ handleNative("cooler","toggle"); });
  server.on("/api/cooler/state",  [](){ handleNative("cooler","state"); });
  server.on("/api/light/on",      [](){ handleNative("light","on"); });
  server.on("/api/light/off",     [](){ handleNative("light","off"); });
  server.on("/api/light/toggle",  [](){ handleNative("light","toggle"); });
  server.on("/api/light/state",   [](){ handleNative("light","state"); });
  server.begin();
  Serial.println("RelayBoard ready");
}

void loop() {
  server.handleClient();
}
