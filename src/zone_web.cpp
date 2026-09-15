/*
 *  Zone status LAN page — direct loop reading, no panel.
 *
 *  Successor to web_status.cpp (kept as the keybus-web reference env) after
 *  the PC1555 was retired. Same idea: the ESP32 serves its own status page,
 *  no MQTT, no broker, no Home Assistant, no cloud. Open http://dsc.local/
 *  (or the IP printed on serial) from any browser on the LAN.
 *
 *  Shows each zone's state and live voltage — the voltage column is the
 *  threshold-verification tool, not decoration. Arming logic is deliberately
 *  absent: this is a sensor, not an alarm.
 *
 *  Build:  cp include/secrets.h.example include/secrets.h   (then edit it)
 *          pio run -e web -t upload -t monitor
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"
#include "zones.h"

WebServer server(80);


// ---------------------------------------------------------------- page ----

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Zone Status</title>
<style>
:root{color-scheme:light dark;--bg:#f6f6f7;--card:#fff;--fg:#16171a;--dim:#6b6f76;
--line:#e3e4e8;--ok:#1a7f4b;--warn:#b8860b;--bad:#c0392b;--okbg:#e7f5ee;--badbg:#fdecea;
--warnbg:#fdf6e3}
@media(prefers-color-scheme:dark){:root{--bg:#0f1012;--card:#17191c;--fg:#e8e9ea;
--dim:#9aa0a6;--line:#2a2d31;--ok:#4ade80;--warn:#fbbf24;--bad:#f87171;
--okbg:#132218;--badbg:#2a1416;--warnbg:#2a2210}}
*{box-sizing:border-box}
body{margin:0;padding:1.25rem;background:var(--bg);color:var(--fg);
font:16px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif}
.wrap{max-width:640px;margin:0 auto}
h1{font-size:1.1rem;margin:0 0 1rem;letter-spacing:.02em;color:var(--dim);font-weight:600}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;
padding:1rem;margin-bottom:1rem}
.state{font-size:1.6rem;font-weight:650;margin:0}
.sub{color:var(--dim);font-size:.9rem;margin:.25rem 0 0}
.zones{display:grid;gap:.5rem}
.z{display:flex;justify-content:space-between;align-items:center;gap:.5rem;
padding:.55rem .7rem;border:1px solid var(--line);border-radius:8px;font-size:.9rem}
.z b{font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.z .mv{color:var(--dim);font-size:.8rem;font-variant-numeric:tabular-nums}
.tag{font-size:.72rem;text-transform:uppercase;letter-spacing:.06em;
padding:.15rem .45rem;border-radius:5px;white-space:nowrap}
.closed{background:var(--okbg);color:var(--ok)}
.open{background:var(--badbg);color:var(--bad)}
.tamper{background:var(--warnbg);color:var(--warn)}
.unknown{background:var(--bg);color:var(--dim)}
.sys{display:flex;flex-wrap:wrap;gap:.4rem 1.25rem;font-size:.88rem;color:var(--dim)}
.sys span b{font-weight:600}
.good{color:var(--ok)}.bad{color:var(--bad)}.warn{color:var(--warn)}
#stale{display:none;color:var(--warn);font-size:.85rem;margin-top:.5rem}
</style></head><body><div class="wrap">
<h1>Zone Status</h1>

<div class="card">
  <p class="state" id="state">Connecting…</p>
  <p class="sub" id="sub">&nbsp;</p>
  <div id="stale">No response from the zone reader.</div>
</div>

<div class="card"><div class="zones" id="zones"></div></div>

<div class="card"><div class="sys" id="sys"></div></div>

<script>
function esc(s){return s.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));}
const cls={closed:'closed',OPEN:'open','SHORT/tamper':'tamper',unknown:'unknown'};
let misses=0;
async function tick(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    const d=await r.json();
    misses=0;document.getElementById('stale').style.display='none';

    const open=d.zones.filter(z=>z.state==='OPEN');
    const tamper=d.zones.filter(z=>z.state==='SHORT/tamper');
    const s=document.getElementById('state');
    if(tamper.length){s.textContent='TAMPER';s.className='state bad';
      document.getElementById('sub').textContent=tamper.map(z=>z.name).join(', ');}
    else if(open.length){s.textContent='Open';s.className='state warn';
      document.getElementById('sub').textContent=open.map(z=>z.name).join(', ');}
    else{s.textContent='All closed';s.className='state good';
      document.getElementById('sub').textContent='Every zone secure';}

    document.getElementById('zones').innerHTML=d.zones.map(z=>
      `<div class="z"><b>${esc(z.name)}</b><span class="mv">${z.mv} mV</span>`+
      `<span class="tag ${cls[z.state]||'unknown'}">${esc(z.state)}</span></div>`).join('');

    document.getElementById('sys').innerHTML=[
      ['Uptime',d.uptime,true],
      ['WiFi',d.rssi+' dBm',d.rssi>-75],
      ['Poll',d.pollMs+' ms',true]
    ].map(([l,v,ok])=>`<span>${l}: <b class="${ok?'good':'bad'}">${esc(String(v))}</b></span>`).join('');
  }catch(e){ if(++misses>2) document.getElementById('stale').style.display='block'; }
}
tick();setInterval(tick,1500);
</script></div></body></html>)HTML";


// ---------------------------------------------------------------- json ----

static String uptimeString() {
  unsigned long s = millis() / 1000;
  char buf[24];
  snprintf(buf, sizeof(buf), "%lud %luh %lum", s / 86400, (s % 86400) / 3600, (s % 3600) / 60);
  return String(buf);
}

static void handleStatus() {
  String j;
  j.reserve(512);
  j += "{\"zones\":[";
  for (uint8_t i = 0; i < ZONE_COUNT; i++) {
    if (i) j += ',';
    j += "{\"n\":";       j += (i + 1);
    j += ",\"name\":\"";  j += kZones[i].name;
    j += "\",\"state\":\""; j += zoneStateName(zoneState(i));
    j += "\",\"mv\":";    j += zoneMv(i);
    j += '}';
  }
  j += "],\"uptime\":\"" + uptimeString() + "\"";
  j += ",\"rssi\":";  j += WiFi.RSSI();
  j += ",\"pollMs\":"; j += ZONE_POLL_MS;
  j += '}';

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}


// --------------------------------------------------------------- setup ----

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();

  zonesBegin();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);   // REQUIRED: GPIO 36 (zone 5) glitches under WiFi power-save
  WiFi.setHostname(MDNS_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to WiFi"));
  while (WiFi.status() != WL_CONNECTED) { Serial.print('.'); delay(500); }
  Serial.println();
  Serial.print(F("IP address: http://"));
  Serial.println(WiFi.localIP());

  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Also at: http://%s.local/\n", MDNS_HOSTNAME);
  }

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.onNotFound([]() { server.send(404, "text/plain", "not found"); });
  server.begin();

  Serial.println(F("Zone reader online."));
}


void loop() {
  zonesPoll([](uint8_t i, ZoneState oldState, ZoneState newState) {
    Serial.printf("[%10lu ms] zone %u  %-16s %s -> %s  (%u mV)\n",
                  millis(), i + 1, kZones[i].name,
                  zoneStateName(oldState), zoneStateName(newState), zoneMv(i));
  });
  server.handleClient();

  // Reconnect WiFi if it drops
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {
    lastCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("WiFi lost, reconnecting"));
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
}
