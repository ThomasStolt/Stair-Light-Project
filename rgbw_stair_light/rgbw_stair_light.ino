// STAIR LIGHT PROJECT - started October 2016
// ==========================================
//
// This project is using NeoPixels to illuminate individual steps of a flight of stairs
// automatically, while someone is walking up (or down) the stairs. The project is based  
// on an ESP8266 as microcontroller, strips of SK2812 as LEDs (should be compatible to
// WS2812) and two SR501 as motion sensors. An ESP8266 is used as the microcontroller.
// I have tried to make it easy to adapt this sketch to your own needs.
// You can e.g. change the number of steps of your stairs (STEPS) as well as
// the 'width' of your stairs, in terms of how many LEDs are you using per step (WIDTH).
// I have written a few animations, much of this code is based on the strandtest code
// example from adafruit, with some adaptations however.
//
// This is pretty much work in progress.
//
// There is some tweaking needed depending on local parameters with the PIR sensors,
// the ANIM_DURATION and the delay after each animation. For me it worked when I set
// the activation time potentiometer to minimum (which results to about 4 seconds of
// the pin set to high), an ANIM_DURATION of 20 seconds (because in my house, one needs
// about 10 seconds to go up the stairs at normal speed) and a delay after each
// animation of 7 seconds. You will have to play around to fit your needs.
//
//
// last update 30.03.2018
//

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include "credentials.h"
#include "birthdays.h"
#include <Ticker.h>
#include <time.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

// Pin Assignment: it turns out that GPIO 15 and 2 influence the boot mode
// of the ESP8266, so they should not be used ever.
//

#define NEOPIXEL_PIN  14          // Pin D5 == GPIO 14 -> NeoPixels
#define PIR1 16                   // Pin D0 == GPIO 16 -> PIR Sensor 1
#define PIR2 4                    // Pin D2 == GPIO 4  -> Pir Sensor 2
#define STEPS 16                  // how many steps do the stairs have?
#define WIDTH 27                  // how many LEDs per step do we have?
#define NUM_LEDS (STEPS * WIDTH)  // how many LEDs do we have overall?
#define ANIM_DURATION 20000       // Duration of each animation (ms), then fade-out
#define POST_ANIM_DELAY_MS 10000  // Delay after animation before next motion trigger (ms)
// Auto-detect CET/CEST: UTC+1 in winter, UTC+2 in summer (last Sun of March – last Sun of October)
static long timezoneOffsetSec(long utc) {
  time_t t = (time_t)utc;
  struct tm *g = gmtime(&t);
  int month = g->tm_mon + 1;   // 1-12
  int day   = g->tm_mday;
  int wday  = g->tm_wday;      // 0=Sun
  int hour  = g->tm_hour;
  // Last Sunday of month = last day minus (wday of last day)
  // For March/October: switch happens at 01:00 UTC on last Sunday
  if (month < 3 || month > 10) return 3600;        // Nov–Feb: CET (UTC+1)
  if (month > 3 && month < 10) return 7200;         // Apr–Sep: CEST (UTC+2)
  // March: CEST starts last Sunday at 01:00 UTC
  int lastSun = 31 - ((5 + wday + 31 - day) % 7);  // last Sunday's date this month
  if (month == 3) {
    if (day < lastSun) return 3600;
    if (day > lastSun) return 7200;
    return (hour < 1) ? 3600 : 7200;
  }
  // October: CET starts last Sunday at 01:00 UTC
  lastSun = 31 - ((2 + wday + 31 - day) % 7);
  if (day < lastSun) return 7200;
  if (day > lastSun) return 3600;
  return (hour < 1) ? 7200 : 3600;
}
#define TIMEZONE_OFFSET_SEC timezoneOffsetSec
// if BRIGHNESS is too small (around 10 or less), the animation appears 'skippy', i.e. not smooth
// that is because there are only a few (10) levels of brighness for each color, so this is normal
#define BRIGHTNESS 255            // limit brightness of the strip
#define USE_SERIAL Serial
#define DEBUG 1   // 1 = Serial monitor shows PIR state and trigger

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);

// Web control: stair automation and manual colour (0–100% per channel)
bool automationOn = true;   // Start: automation on (can be turned off via web)
uint8_t manual_r = 0, manual_g = 0, manual_b = 0, manual_w = 0;  // 0–100 %

// If > 0: animations (e.g. web "Go") run only for this duration (ms), else ANIM_DURATION
uint32_t g_animDurationOverrideMs = 0;

// Last NTP time (UTC) from main loop; used by /api/time for Web UI date/time display
volatile long g_lastNtpTime = 0;
// First valid NTP time after boot (for "last reboot" display in Web UI)
static long s_bootTimeUtc = 0;

// Reset reason / exception info cached once at boot (before WiFi changes them)
static String s_resetReason;
static String s_resetInfo;

// WiFi reconnect counter (incremented on every re-connect after the first one)
static uint32_t g_wifiReconnectCount = 0;
static WiFiEventHandler g_wifiReconnectHandler;

// Last 5 motion events (direction + animation) for Web UI log
#define MOTION_LOG_SIZE 5
struct MotionLogEntry { char dir[6]; uint8_t anim_id; long timestamp; };
MotionLogEntry motionLog[MOTION_LOG_SIZE];
uint8_t motionLogHead = 0;
uint8_t motionLogCount = 0;

void addMotionLog(const char* dir, uint8_t anim_id) {
  strncpy(motionLog[motionLogHead].dir, dir, 5);
  motionLog[motionLogHead].dir[5] = '\0';
  motionLog[motionLogHead].anim_id = anim_id;
  motionLog[motionLogHead].timestamp = g_lastNtpTime;
  motionLogHead = (motionLogHead + 1) % MOTION_LOG_SIZE;
  if (motionLogCount < MOTION_LOG_SIZE) motionLogCount++;
}

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800);



int gammaw[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

// Firmware version – shown in web UI footer
#define FW_VERSION "1.0.0"

// Night mode parameters – defined here so parking.h can use them
#define NIGHT_HOUR_START      1   // 1:00
#define NIGHT_HOUR_END        6   // 6:00 (exclusive)
#define NIGHT_HOUR_START_STR  "1"
#define NIGHT_HOUR_END_STR    "6"
#define NIGHT_BRIGHTNESS_MAX  50  // max red value during night (≈ 20%)
#define NIGHT_BRIGHTNESS_MIN  10  // min red value (never fully off)

// Called in animations and wait loops so OTA and web server stay responsive during animation
void handleNetwork(void);

#include "parking.h"

void handleNetwork(void) {
  ArduinoOTA.handle();
  server.handleClient();
}

WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", ((0*60*60)+(0*60))); // CET = GMT + 1:00

Ticker secondTick;
volatile int watchdogCount = 0;

void ISRwatchdog() {
  watchdogCount++;
  if (watchdogCount == 360) {
    Serial.println();
    Serial.println("the watchdog bites!!!");
    ESP.restart();
  }

}

// Apply manual colour (R,G,B,W 0–100%) to strip (with gamma).
// 1–100% mapped to gamma index 25–255 so 10% is already visible.
static uint8_t manualPctToGamma(uint8_t pct) {
  if (pct == 0) return 0;
  int idx = 25 + (int)((unsigned long)pct * 230 / 100);
  if (idx > 255) idx = 255;
  return gammaw[idx];
}
void applyManualColor() {
  uint8_t r = manualPctToGamma(manual_r);
  uint8_t g = manualPctToGamma(manual_g);
  uint8_t b = manualPctToGamma(manual_b);
  uint8_t w = manualPctToGamma(manual_w);
  setAll(r, g, b, w);
  strip.show();
}

// Statisches Frontend (cachebar), State und Aktionen per API
void handleIndex() {
  server.sendHeader(F("Cache-Control"), F("public, max-age=3600"));
  server.send_P(200, PSTR("text/html"), PSTR(
    "<!DOCTYPE html><html><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>Stair Light</title><style>"
    "body{font-family:sans-serif;margin:0;background:#1a1a1a;color:#eee;display:flex;flex-direction:column;align-items:center;min-height:100vh;padding:1.5rem 1rem;box-sizing:border-box;}"
    "main{text-align:center;max-width:420px;width:100%;} h1{font-size:1.3rem;margin:0 0 1rem;}"
    ".row{display:flex;align-items:center;justify-content:center;gap:0.4rem;margin:0.5rem 0;flex-wrap:nowrap;overflow-x:auto;padding:2px 0;}"
    "button.btn{padding:0.9rem 1.2rem;margin:2px;border-radius:10px;font-size:1.1rem;cursor:pointer;border:1px solid rgba(0,0,0,0.25);min-height:2.6em;flex-shrink:0;"
    "box-shadow:inset 0 1px 0 rgba(255,255,255,0.18),0 3px 6px rgba(0,0,0,0.35);text-shadow:0 1px 1px rgba(0,0,0,0.3);transition:box-shadow .1s,transform .1s;}"
    "button.btn:active{box-shadow:inset 0 2px 6px rgba(0,0,0,0.4);transform:translateY(1px);}"
    ".minus,.plus,.pctbtn{background:linear-gradient(180deg,#3a4a5c 0%,#2a3a4a 45%,#1a2a38 100%);color:#8cf;}"
    ".onoff{background:linear-gradient(180deg,#555 0%,#444 45%,#333 100%);color:#fff;}"
    ".auto{background:linear-gradient(180deg,#3a5a3a 0%,#2a4a2a 45%,#1a3a1a 100%);color:#9f9;}"
    ".auto.off{background:linear-gradient(180deg,#5a3a3a 0%,#4a2a2a 45%,#3a1a1a 100%);color:#f99;}"
    ".reboot{background:linear-gradient(180deg,#884422 0%,#663300 45%,#442200 100%);color:#ffc;}"
    ".go{background:linear-gradient(180deg,#3a6a3a 0%,#2a5a2a 45%,#1a4a1a 100%);color:#9f9;}"
    "button.btn.allpct{padding:0.5rem 0.5rem;min-width:0;font-size:0.95rem;}"
    "span.pct{display:inline-block;min-width:2.2em;text-align:left;} .led{width:14px;height:14px;border-radius:50%;display:inline-block;margin-right:0.35rem;box-shadow:0 0 6px currentColor;}"
    ".led-r{background:#e00;} .led-g{background:#0a0;} .led-b{background:#06f;} .led-w{background:#eee;}"
    "table{width:100%;border-collapse:collapse;font-size:0.85rem;margin-top:0.3rem;} th,td{border:1px solid #444;padding:0.25rem 0.4rem;text-align:left;} th{background:#333;}"
    "</style></head><body><main><h1>Stair Light</h1>"
    "<p style=margin-bottom:0.5rem;font-size:0.95rem;><span id=dateDisplay>--</span> <span id=timeDisplay>--</span></p>"
    "<p style=margin-bottom:0.3rem;font-size:0.85rem;>Uptime: <span id=uptimeDisplay>--</span> &nbsp; Last reboot: <span id=lastRebootDisplay>--</span></p>"
    "<p style=margin-bottom:1rem;><b>Stair automation</b><br>"
    "<button type=button class=\"btn auto\" id=autoOn>On</button> <button type=button class=\"btn auto off\" id=autoOff>Off</button> &rarr; <b id=autoStatus>–</b></p>"
    "<p id=nightBadge style=\"display:none;margin:-0.5rem 0 0.8rem;padding:0.4rem 0.8rem;border-radius:8px;background:rgba(180,40,40,0.25);border:1px solid #a33;font-size:0.9rem;color:#f88;\">Night mode active (" NIGHT_HOUR_START_STR ":00 – " NIGHT_HOUR_END_STR ":00)</p>"
    "<div class=row><span class=\"led led-r\"></span><button type=button class=\"btn minus\" data-c=r data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togR>Off</button><button type=button class=\"btn plus\" data-c=r data-a=plus>+10%</button><span id=pctR class=pct>0%</span></div>"
    "<div class=row><span class=\"led led-g\"></span><button type=button class=\"btn minus\" data-c=g data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togG>Off</button><button type=button class=\"btn plus\" data-c=g data-a=plus>+10%</button><span id=pctG class=pct>0%</span></div>"
    "<div class=row><span class=\"led led-b\"></span><button type=button class=\"btn minus\" data-c=b data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togB>Off</button><button type=button class=\"btn plus\" data-c=b data-a=plus>+10%</button><span id=pctB class=pct>0%</span></div>"
    "<div class=row><span class=\"led led-w\"></span><button type=button class=\"btn minus\" data-c=w data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togW>Off</button><button type=button class=\"btn plus\" data-c=w data-a=plus>+10%</button><span id=pctW class=pct>0%</span></div>"
    "<p style=margin-top:0.8rem;margin-bottom:0.3rem;><b>All</b></p>"
    "<div class=row style=align-items:center;><button type=button class=\"btn minus allpct\" data-all=0>0%</button>"
    "<button type=button class=\"btn pctbtn allpct\" data-all=25>25%</button><button type=button class=\"btn pctbtn allpct\" data-all=50>50%</button>"
    "<button type=button class=\"btn pctbtn allpct\" data-all=75>75%</button><button type=button class=\"btn plus allpct\" data-all=100>100%</button></div>"
    "<p style=margin-top:0.8rem;><button type=button class=\"btn reboot\" id=rebootBtn>Reboot</button></p>"
    "<p style=margin-top:1rem;><b>Last 5 motions</b></p>"
    "<table><thead><tr><th>Time</th><th>Direction</th><th>Animation</th></tr></thead><tbody id=motionLogBody></tbody></table>"
    "<p style=margin-top:1rem;><b>Memory status</b></p>"
    "<table><thead><tr><th>Type</th><th>Total</th><th>Used</th><th>Usage</th></tr></thead><tbody id=memoryBody></tbody></table>"
    "<p style=margin-top:1rem;><b>CPU / Runtime</b></p>"
    "<table><thead><tr><th>Key</th><th>Value</th></tr></thead><tbody id=cpuBody></tbody></table>"
    "<p style=margin-top:1rem;><b>WiFi</b></p>"
    "<table><thead><tr><th>Key</th><th>Value</th></tr></thead><tbody id=wifiBody></tbody></table>"
    "<p style=margin-top:1rem;><b>Animation (10 s)</b><br>"
    "<select id=animSelect style=padding:0.4rem;background:#333;color:#eee;border-radius:6px;margin-right:0.5rem;>"
    "<option value=1>Random fade</option><option value=2>Rainbow</option><option value=3>White ramp</option>"
    "<option value=4>Star sparkle</option><option value=5>Birthday</option><option value=6>Night (red breathing)</option></select>"
    "<button type=button class=\"btn go\" id=playAnim>Go</button></p>"
    "<p style=margin-top:1.5rem;font-size:0.75rem;color:#666;>Firmware v" FW_VERSION "</p></main><script>"
    "function load(){"
    "Promise.all([fetch('/api/state').then(function(r){return r.json();}), fetch('/api/time').then(function(r){return r.json();}), fetch('/api/log').then(function(r){return r.json();}), fetch('/api/memory').then(function(r){return r.json();}), fetch('/api/sysinfo').then(function(r){return r.json();})]).then(function(arr){"
    "var d=arr[0], t=arr[1], log=arr[2], mem=arr[3], sys=arr[4];"
    "document.getElementById('autoStatus').textContent=d.auto? 'On':'Off';"
    "document.getElementById('nightBadge').style.display=d.night? 'block':'none';"
    "var ch=['r','g','b','w'], id=['pctR','pctG','pctB','pctW'], tog=['togR','togG','togB','togW'];"
    "for(var i=0;i<4;i++){ document.getElementById(id[i]).textContent=d[ch[i]]+'%'; document.getElementById(tog[i]).textContent=d[ch[i]]>0?'On':'Off'; }"
    "document.getElementById('dateDisplay').textContent=t.date||'--'; document.getElementById('timeDisplay').textContent=t.time||'--';"
    "var ms=t.uptime_ms||0; var d=Math.floor(ms/86400000), h=Math.floor((ms%86400000)/3600000), m=Math.floor((ms%3600000)/60000);"
    "document.getElementById('uptimeDisplay').textContent=(d>0? d+'d ':'')+h+'h '+m+'m';"
    "document.getElementById('lastRebootDisplay').textContent=t.last_reboot||'--';"
    "var tb=document.getElementById('motionLogBody'); tb.innerHTML=''; for(var i=0;i<log.length;i++){ var r=document.createElement('tr'); r.innerHTML='<td>'+log[i].time+'</td><td>'+log[i].dir+'</td><td>'+log[i].anim+'</td>'; tb.appendChild(r); }"
    "function fmt(n){ return n>=1024 ? (n/1024).toFixed(1)+' KB' : n+' B'; }"
    "var mb=document.getElementById('memoryBody'); mb.innerHTML='<tr><td>Heap (RAM)</td><td>'+fmt(mem.heap_total)+'</td><td>'+fmt(mem.heap_used)+'</td><td>'+mem.heap_pct+'%</td></tr><tr><td>Flash</td><td>'+fmt(mem.flash_size)+'</td><td>'+fmt(mem.flash_used)+'</td><td>'+mem.flash_pct+'%</td></tr><tr><td>RTC</td><td>'+fmt(mem.rtc_total)+'</td><td>'+fmt(mem.rtc_used)+'</td><td>'+mem.rtc_pct+'%</td></tr>';"
    "var rsn=sys.reset_reason||'--'; var exRow=(rsn.indexOf('xception')>=0)?'<tr><td>Exception info</td><td>'+sys.reset_info+'</td></tr>':'';"
    "document.getElementById('cpuBody').innerHTML='<tr><td>CPU freq</td><td>'+sys.cpu_mhz+' MHz</td></tr><tr><td>Reset reason</td><td>'+rsn+'</td></tr>'+exRow;"
    "function rssiQ(v){ if(v>=-50)return 'Excellent'; if(v>=-60)return 'Good'; if(v>=-70)return 'Fair'; return 'Poor'; }"
    "var rssi=sys.rssi||0;"
    "document.getElementById('wifiBody').innerHTML='<tr><td>SSID</td><td>'+sys.ssid+'</td></tr><tr><td>BSSID</td><td>'+sys.bssid+'</td></tr><tr><td>IP</td><td>'+sys.ip+'</td></tr><tr><td>Gateway</td><td>'+sys.gateway+'</td></tr><tr><td>DNS</td><td>'+sys.dns+'</td></tr><tr><td>Channel</td><td>'+sys.channel+'</td></tr><tr><td>RSSI</td><td>'+rssi+' dBm ('+rssiQ(rssi)+')</td></tr><tr><td>Reconnects</td><td>'+sys.reconnects+'</td></tr>';"
    "});}"
    "function post(url,body){ return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body||''}).then(load);}"
    "document.getElementById('autoOn').onclick=function(){ post('/api/auto','on=1'); };"
    "document.getElementById('autoOff').onclick=function(){ post('/api/auto','on=0'); };"
    "document.querySelectorAll('.allpct').forEach(function(btn){ btn.onclick=function(){ post('/api/color','all='+btn.getAttribute('data-all')); }; });"
    "['minus','plus'].forEach(function(a){ var bt=document.querySelectorAll('[data-a='+a+']');"
    "for(var i=0;i<bt.length;i++){ (function(act,btn){ btn.onclick=function(){ var c=btn.getAttribute('data-c'); post('/api/color','c='+c+'&a='+act); }; })(a,bt[i]); }"
    "});"
    "['togR','togG','togB','togW'].forEach(function(id){ var c=id.replace('tog','').toLowerCase();"
    "document.getElementById(id).onclick=function(){ post('/api/color','c='+c+'&a=toggle'); }; });"
    "document.getElementById('playAnim').onclick=function(){ var v=document.getElementById('animSelect').value; post('/api/play','anim='+v); };"
    "document.getElementById('rebootBtn').onclick=function(){ this.disabled=true; this.textContent='Rebooting...'; post('/api/reboot').then(function(){ setTimeout(function(){ location.reload(); }, 4000); }); };"
    "load(); setInterval(load, 1000);"
    "</script></body></html>"
  ));
}

void handleApiState() {
  String json = "{\"auto\":";
  json += automationOn ? "1" : "0";
  json += ",\"r\":"; json += manual_r;
  json += ",\"g\":"; json += manual_g;
  json += ",\"b\":"; json += manual_b;
  json += ",\"w\":"; json += manual_w;
  json += ",\"night\":"; json += isNightMode(g_lastNtpTime) ? "1" : "0";
  json += "}";
  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.send(200, F("application/json"), json);
}

void handleApiAuto() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  if (server.hasArg(F("on"))) {
    automationOn = (server.arg(F("on")).toInt() != 0);
    if (!automationOn) applyManualColor();
    else { setAll(0, 0, 0, 0); strip.show(); }
  }
  server.send(204);
}

void handleApiColor() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  // Set or adjust all channels (0–100): all=<value> or all=1&a=minus10|minus1|plus1|plus10
  if (server.hasArg(F("all"))) {
    int pct;
    if (server.hasArg(F("a"))) {
      String ax = server.arg(F("a"));
      int cur = (int)manual_r;
      if (ax == F("minus10")) pct = cur - 10;
      else if (ax == F("minus1")) pct = cur - 1;
      else if (ax == F("plus1")) pct = cur + 1;
      else if (ax == F("plus10")) pct = cur + 10;
      else { server.send(400, F("text/plain"), F("a=minus10|minus1|plus1|plus10")); return; }
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
    } else {
      pct = server.arg(F("all")).toInt();
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
    }
    manual_r = manual_g = manual_b = manual_w = (uint8_t)pct;
    if (!automationOn) applyManualColor();
    server.send(204);
    return;
  }
  if (!server.hasArg(F("c")) || !server.hasArg(F("a"))) { server.send(400, F("text/plain"), F("c and a required")); return; }
  String c = server.arg(F("c"));
  String a = server.arg(F("a"));
  uint8_t* v = nullptr;
  if (c == F("r")) v = &manual_r; else if (c == F("g")) v = &manual_g; else if (c == F("b")) v = &manual_b; else if (c == F("w")) v = &manual_w;
  if (!v) { server.send(400, F("text/plain"), F("c=r|g|b|w")); return; }
  if (a == F("minus")) { *v = (*v <= 10) ? 0 : (*v - 10); }
  else if (a == F("plus"))  { *v = (*v >= 90) ? 100 : (*v + 10); }
  else if (a == F("toggle")) { *v = (*v > 0) ? 0 : 50; }
  if (!automationOn) applyManualColor();
  server.send(204);
}

void handleApiAlloff() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  manual_r = manual_g = manual_b = manual_w = 0;
  setAll(0, 0, 0, 0);
  strip.show();
  server.send(204);
}

// Play animation for 10 s (1–5 as below, 6=night red breathing)
void handleApiPlay() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  if (!server.hasArg(F("anim"))) { server.send(400, F("text/plain"), F("anim=1..6")); return; }
  int anim = server.arg(F("anim")).toInt();
  if (anim < 1 || anim > 6) { server.send(400, F("text/plain"), F("anim 1..6")); return; }

  if (anim == 6) {
    g_animDurationOverrideMs = 10000;
    nightAnimation("UP");
    g_animDurationOverrideMs = 0;
    setAll(0, 0, 0, 0);
    strip.show();
    server.send(204);
    return;
  }

  g_animDurationOverrideMs = 10000;
  if (anim == 1) simpleFadeToRandom(F("UP"));
  else if (anim == 2) rainbowSteps(F("UP"));
  else if (anim == 3) FadeToFullBrightness(F("UP"));
  else if (anim == 4) starSparkle(F("UP"));
  else if (anim == 5) birthday(F("UP"));
  g_animDurationOverrideMs = 0;
  server.send(204);
}

// GET /api/time – JSON with date, time, uptime_ms, last_reboot (local, from NTP) for Web UI
void handleApiTime() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  String json = "{\"date\":\"";
  if (g_lastNtpTime <= 0) {
    json += "--\",\"time\":\"--\"";
  } else {
    time_t t = (time_t)(g_lastNtpTime + TIMEZONE_OFFSET_SEC(g_lastNtpTime));
    struct tm *tm = gmtime(&t);
    if (!tm) {
      json += "--\",\"time\":\"--\"";
    } else {
      char buf[24];
      snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
      json += buf;
      json += "\",\"time\":\"";
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
      json += buf;
    }
  }
  json += "\",\"uptime_ms\":";
  json += (unsigned long)millis();
  json += ",\"last_reboot\":\"";
  if (s_bootTimeUtc <= 0) {
    json += "--\"}";
  } else {
    time_t bt = (time_t)(s_bootTimeUtc + TIMEZONE_OFFSET_SEC(s_bootTimeUtc));
    struct tm *bm = gmtime(&bt);
    if (!bm) {
      json += "--\"}";
    } else {
      char buf[24];
      snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
               bm->tm_year + 1900, bm->tm_mon + 1, bm->tm_mday,
               bm->tm_hour, bm->tm_min, bm->tm_sec);
      json += buf;
      json += "\"}";
    }
  }
  server.send(200, F("application/json"), json);
}

// POST /api/reboot – reboot ESP (send 204 then restart after short delay)
void handleApiReboot() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  server.send(204);
  server.handleClient();
  delay(300);
  ESP.restart();
}

// GET /api/log – JSON array of last 5 motions: { dir, anim, time } (newest first)
void handleApiLog() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  String json = "[";
  const char* animNames[] = { "", "Random fade", "Rainbow", "White ramp", "Star sparkle", "Birthday" };
  for (uint8_t i = 0; i < motionLogCount; i++) {
    int idx = (motionLogHead - 1 - i + MOTION_LOG_SIZE) % MOTION_LOG_SIZE;
    if (i > 0) json += ",";
    json += "{\"dir\":\""; json += motionLog[idx].dir;
    json += "\",\"anim\":\""; json += (motionLog[idx].anim_id <= 5) ? animNames[motionLog[idx].anim_id] : "";
    json += "\",\"time\":\"";
    if (motionLog[idx].timestamp <= 0) {
      json += "--:--";
    } else {
      time_t t = (time_t)(motionLog[idx].timestamp + TIMEZONE_OFFSET_SEC(motionLog[idx].timestamp));
      struct tm *tm = gmtime(&t);
      if (tm) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", tm->tm_hour, tm->tm_min);
        json += buf;
      } else json += "--:--";
    }
    json += "\"}";
  }
  json += "]";
  server.send(200, F("application/json"), json);
}

// GET /api/memory – JSON with heap/flash stats (total, used, %) for Web UI status table
#ifndef ESP8266_HEAP_TOTAL
#define ESP8266_HEAP_TOTAL 80192u  // typical user heap on NodeMCU/ESP-12
#endif
void handleApiMemory() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  uint32_t heapFree = ESP.getFreeHeap();
  uint32_t heapTotal = ESP8266_HEAP_TOTAL;
  uint32_t heapUsed = (heapTotal > heapFree) ? (heapTotal - heapFree) : 0;
  uint32_t heapPct = (heapTotal > 0) ? (heapUsed * 100 / heapTotal) : 0;
  uint8_t heapFrag = ESP.getHeapFragmentation();
  uint32_t heapMaxBlock = ESP.getMaxFreeBlockSize();
  uint32_t sketchSize = ESP.getSketchSize();
  uint32_t sketchFree = ESP.getFreeSketchSpace();
  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t flashUsed = sketchSize;
  uint32_t flashPct = (flashSize > 0) ? (flashUsed * 100 / flashSize) : 0;
  // RTC: 768 bytes total on ESP8266 (persists in deep sleep); no API for used, report 0 if unused
  const uint32_t rtcTotal = 768u;
  uint32_t rtcUsed = 0;
  uint32_t rtcPct = 0;
  String json = "{\"heap_total\":";
  json += heapTotal;
  json += ",\"heap_used\":";
  json += heapUsed;
  json += ",\"heap_pct\":";
  json += heapPct;
  json += ",\"heap_free\":";
  json += heapFree;
  json += ",\"heap_frag\":";
  json += heapFrag;
  json += ",\"heap_max_block\":";
  json += heapMaxBlock;
  json += ",\"flash_size\":";
  json += flashSize;
  json += ",\"flash_used\":";
  json += flashUsed;
  json += ",\"flash_pct\":";
  json += flashPct;
  json += ",\"rtc_total\":";
  json += rtcTotal;
  json += ",\"rtc_used\":";
  json += rtcUsed;
  json += ",\"rtc_pct\":";
  json += rtcPct;
  json += ",\"sketch_free\":";
  json += sketchFree;
  json += "}";
  server.send(200, F("application/json"), json);
}

// Returns true if current date (local, NTP + CET/CEST offset) is listed in birthdays.h.
bool isBirthdayToday(long unixTimeUtc) {
  if (BIRTHDAY_COUNT == 0) return false;
  time_t t = (time_t)(unixTimeUtc + TIMEZONE_OFFSET_SEC(unixTimeUtc));
  struct tm *tm = gmtime(&t);
  if (!tm) return false;
  int month = tm->tm_mon + 1;
  int day = tm->tm_mday;
  for (int i = 0; i < BIRTHDAY_COUNT; i++) {
    if ((int)BIRTHDAYS[i][0] == month && (int)BIRTHDAYS[i][1] == day)
      return true;
  }
  return false;
}

// GET /api/sysinfo – CPU freq, reset reason/exception, WiFi details, reconnect counter
void handleApiSysinfo() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  // Sanitise reset_info: replace " so it doesn't break the JSON string
  String ri = s_resetInfo;
  ri.replace("\"", "'");
  String json = "{\"cpu_mhz\":";
  json += ESP.getCpuFreqMHz();
  json += ",\"reset_reason\":\""; json += s_resetReason;     json += "\"";
  json += ",\"reset_info\":\"";   json += ri;                json += "\"";
  json += ",\"rssi\":";           json += WiFi.RSSI();
  json += ",\"ssid\":\"";         json += WiFi.SSID();       json += "\"";
  json += ",\"bssid\":\"";        json += WiFi.BSSIDstr();   json += "\"";
  json += ",\"ip\":\"";           json += WiFi.localIP().toString();      json += "\"";
  json += ",\"gateway\":\"";      json += WiFi.gatewayIP().toString();    json += "\"";
  json += ",\"dns\":\"";          json += WiFi.dnsIP().toString();        json += "\"";
  json += ",\"channel\":";        json += WiFi.channel();
  json += ",\"reconnects\":";     json += g_wifiReconnectCount;
  json += "}";
  server.send(200, F("application/json"), json);
}

bool isNightMode(long unixTimeUtc) {
  if (unixTimeUtc <= 0) return false;
  time_t t = (time_t)(unixTimeUtc + TIMEZONE_OFFSET_SEC(unixTimeUtc));
  struct tm *tm = gmtime(&t);
  if (!tm) return false;
  int hour = tm->tm_hour;
  return (hour >= NIGHT_HOUR_START && hour < NIGHT_HOUR_END);
}

// Soft red "breathing" (min → max → min), cycle ~10 s; never fully off
void updateNightRed() {
  unsigned long phase = millis() % 10000uL;  // 10 s cycle
  int pct;
  if (phase < 5000uL)
    pct = (int)(phase * 100 / 5000);
  else
    pct = (int)((10000uL - phase) * 100 / 5000);
  uint8_t r = (uint8_t)(NIGHT_BRIGHTNESS_MIN + (unsigned long)(NIGHT_BRIGHTNESS_MAX - NIGHT_BRIGHTNESS_MIN) * (unsigned long)pct / 100);
  setAll(r, 0, 0, 0);
  strip.show();
}

void setup() {
  // Setting up the serial line
  Serial.begin(115200);
  USE_SERIAL.setDebugOutput(true);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  // Cache reset reason/info before WiFi stack overwrites them
  s_resetReason = ESP.getResetReason();
  s_resetInfo   = ESP.getResetInfo();

  // Count WiFi re-connects (skip the very first connection at boot)
  g_wifiReconnectHandler = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected&) {
    static bool first = true;
    if (first) { first = false; return; }
    g_wifiReconnectCount++;
  });

  // Turn off LEDs immediately (before WiFi/OTA – otherwise strip state undefined)
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  setAll(0, 0, 0, 0);
  strip.show();

  // secondTick.attach(1,ISRwatchdog);


  for( uint8_t t = 4; t > 0; t-- ) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  // ========================================================================================
  // WiFi: SSID and password from credentials.h (in .gitignore).
  // ========================================================================================
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  const unsigned long wifiTimeout = 10000;  // 10 s, then continue without WiFi (test bed)
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart) < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No WiFi (timeout). Continuing without network – PIR/LED still work.");
  }
  // print MAC address, uncomment if needed
  // byte mac[6];
  // WiFi.macAddress(mac);
  // Serial.print("MAC: ");
  // for (int i = 0; i < 5; i++) {
  //   Serial.print(mac[i], HEX);
  //   Serial.print(":");
  // }
  // Serial.println(mac[5], HEX);
  // ========================================================================================
  // ========================================================================================

  
  // OTA: you push new firmware from Cursor/PC (arduino-cli upload -p <ESP-IP>).
  // ESP does not fetch anything from a server by itself.
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.onStart([]() {
      Serial.printf("OTA start – free heap: %u bytes\n", ESP.getFreeHeap());
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA end – waiting 2 s before restart (so OK reaches PC).");
      delay(2000);  // Prevents timeout in espota: ESP must send OK before restarting
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive");
      else if (error == OTA_END_ERROR) Serial.println("End");
    });
    ArduinoOTA.begin();
#if DEBUG
  Serial.println("OTA ready. Upload from PC: arduino-cli upload -p " + WiFi.localIP().toString() + " ...");
#endif
  }

  // Web server: frontend cacheable (GET /), API GET/POST
  server.on(F("/"), handleIndex);
  server.on(F("/index.html"), handleIndex);
  server.on(F("/api/state"), HTTP_GET, handleApiState);
  server.on(F("/api/auto"), HTTP_POST, handleApiAuto);
  server.on(F("/api/color"), HTTP_POST, handleApiColor);
  server.on(F("/api/alloff"), HTTP_POST, handleApiAlloff);
  server.on(F("/api/play"), HTTP_POST, handleApiPlay);
  server.on(F("/api/time"), HTTP_GET, handleApiTime);
  server.on(F("/api/reboot"), HTTP_POST, handleApiReboot);
  server.on(F("/api/log"), HTTP_GET, handleApiLog);
  server.on(F("/api/memory"),  HTTP_GET,  handleApiMemory);
  server.on(F("/api/sysinfo"), HTTP_GET,  handleApiSysinfo);
  server.onNotFound([]() { server.send(404, F("text/plain"), F("Not Found")); });
  server.begin();
#if DEBUG
  Serial.println("Web server: http://" + WiFi.localIP().toString());
#endif

  // Strip already initialised at start; here just ensure off
  setAll(0, 0, 0, 0);
  strip.show();

  // initialise the random generator
  randomSeed(ESP.getCycleCount());
  
  pinMode(PIR1, INPUT);
  pinMode(PIR2, INPUT);

#if DEBUG
  Serial.println();
  Serial.println("=== Setup complete ===");
  Serial.println("LEDs off. PIR1 = D0/GPIO16 (up), PIR2 = D2/GPIO4 (down).");
  Serial.println("Waiting for motion... (PIR=HIGH = triggered)");
  Serial.println("================================");
#endif

  // Boot signal: 3× green blink (1 s on, 1 s pause), 50% brightness
  for (int b = 0; b < 3; b++) {
    setAll(0, 127, 0, 0);   // Green, 50% (127/255)
    strip.show();
    delay(1000);
    setAll(0, 0, 0, 0);
    strip.show();
    delay(1000);
  }
}
  
void loop() {
  Serial.println("");
  int count = 0;
  String dir = "";
  int currenttime = 0;
  int lastPir1 = -1, lastPir2 = -1;  // for debug: detect change
  static int lastAnimation = 0;      // 0 = none yet, 1–4 = last chosen animation (no repeat)
  static bool wasNightMode = false;
  while (true) {
    handleNetwork();
    watchdogCount = 0;
    bool night = isNightMode(currenttime);

    // 23–6 h: night mode; strip stays off when idle, PIR triggers nightAnimation below
    if (night) {
      if (!wasNightMode) {
        setAll(0, 0, 0, 0);
        strip.show();
        wasNightMode = true;
      }
    } else {
      if (wasNightMode) {
        setAll(0, 0, 0, 0);
        strip.show();
        wasNightMode = false;
      }
    }

    int pir1 = digitalRead(PIR1);
    int pir2 = digitalRead(PIR2);

#if DEBUG
    // Every ~2 s: print PIR state (heartbeat)
    if (count % 20 == 0) {
      Serial.printf("[%lu] PIR1=%s PIR2=%s  (PIR1=UP, PIR2=DOWN)\n",
                    millis() / 1000, pir1 == HIGH ? "HIGH" : "low ", pir2 == HIGH ? "HIGH" : "low ");
    }
    // Report immediately on state change
    if (pir1 != lastPir1 || pir2 != lastPir2) {
      Serial.printf("      PIR changed -> PIR1=%s PIR2=%s\n", pir1 == HIGH ? "HIGH" : "low ", pir2 == HIGH ? "HIGH" : "low ");
      lastPir1 = pir1;
      lastPir2 = pir2;
    }
#endif

    if (pir1 == HIGH) { dir = "UP"; }
    if (pir2 == HIGH) { dir = "DOWN"; }

    if (night && dir != "") {
#if DEBUG
      Serial.println(">>> PIR triggered (night): " + dir + " -> night animation");
#endif
      addMotionLog(dir.c_str(), 6);
      nightAnimation(dir);
      for (unsigned long t = millis(); millis() - t < (unsigned long)POST_ANIM_DELAY_MS; ) {
        handleNetwork();
        delay(100);
      }
    } else if (!night && automationOn && dir != "") {
#if DEBUG
      Serial.println(">>> PIR triggered: " + dir + " -> starting animation");
#endif
      if (isBirthdayToday(currenttime)) {
        addMotionLog(dir.c_str(), 5);  // 5 = Birthday
        birthday(dir);
        // 10 s delay after birthday animation (same as others)
        for (unsigned long t = millis(); millis() - t < (unsigned long)POST_ANIM_DELAY_MS; ) {
          handleNetwork();
          delay(100);
        }
      } else {
        // Random 1–4, but not same animation as last time
        int choice;
        do {
          choice = random(1, 5);
        } while (choice == lastAnimation);
        lastAnimation = choice;
        addMotionLog(dir.c_str(), (uint8_t)choice);

        switch (choice) {
          case 1:
            simpleFadeToRandom(dir);
            break;
          case 2:
            rainbowSteps(dir);
            break;
          case 3:
            FadeToFullBrightness(dir);
            break;
          case 4:
            starSparkle(dir);
            break;
        }
        // 10 s delay after animation (OTA and web stay available)
        for (unsigned long t = millis(); millis() - t < (unsigned long)POST_ANIM_DELAY_MS; ) {
          handleNetwork();
          delay(100);
        }
      }
    }
    dir = "";

    if (count++ > 100) {
      currenttime = ntpClient.getUnixTime();
      g_lastNtpTime = currenttime;
      if (s_bootTimeUtc == 0 && currenttime > 0) s_bootTimeUtc = currenttime;
#if DEBUG
      Serial.printf("NTP time: %d\n", currenttime);
#endif
      count = 0;
    }
    yield();
    delay(100);
  }
}

