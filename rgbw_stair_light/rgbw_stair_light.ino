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
#define ANIM_DURATION 20000       // Dauer jeder Animation (ms), danach Fade-Out
#define POST_ANIM_DELAY_MS 10000  // Pause nach Animation, bis wieder auf Bewegung gewartet wird
#define TIMEZONE_OFFSET_SEC (1*3600)  // CET = UTC+1; für MEZ 3600, für MESZ 7200
// if BRIGHNESS is too small (around 10 or less), the animation appears 'skippy', i.e. not smooth
// that is because there are only a few (10) levels of brighness for each color, so this is normal
#define BRIGHTNESS 255            // limit brightness of the strip
#define USE_SERIAL Serial
#define DEBUG 1   // 1 = Serieller Monitor zeigt PIR-Zustand und Trigger

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);

// Web-Steuerung: Treppenautomatik und manuelle Farbe (0–100 % pro Kanal)
bool automationOn = false;  // Start: Automatik aus (per Web wieder anstellbar)
uint8_t manual_r = 0, manual_g = 0, manual_b = 0, manual_w = 0;  // 0–100 %

// Wenn > 0: Animationen (z. B. per Web „Go“) laufen nur so lange (ms), sonst ANIM_DURATION
uint32_t g_animDurationOverrideMs = 0;

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

// Wird in Animationen und Warteschleifen aufgerufen, damit OTA und Web-Server auch während Animation reagieren
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

// Manuelle Farbe (R,G,B,W 0–100 %) auf den Strip anwenden (mit Gamma).
// 1–100 % werden auf Gamma-Index 25–255 abgebildet, damit schon 10 % sichtbar sind.
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
    "button.btn{padding:0.9rem 1.2rem;margin:2px;border-radius:10px;font-size:1.1rem;cursor:pointer;border:none;min-height:2.6em;flex-shrink:0;}"
    ".minus{background:#2a3a4a;color:#8cf;} .plus{background:#2a3a4a;color:#8cf;} .onoff{background:#444;color:#fff;}"
    ".alloff{background:#a00;color:#fff;margin-top:1rem;} .auto{background:#2a4a2a;color:#9f9;} .auto.off{background:#4a2a2a;color:#f99;}"
    "span.pct{display:inline-block;min-width:2.2em;text-align:left;} .led{width:14px;height:14px;border-radius:50%;display:inline-block;margin-right:0.35rem;box-shadow:0 0 6px currentColor;}"
    ".led-r{background:#e00;} .led-g{background:#0a0;} .led-b{background:#06f;} .led-w{background:#eee;}"
    "</style></head><body><main><h1>Treppenlicht</h1>"
    "<p style=margin-bottom:1rem;><b>Treppenautomatik</b><br>"
    "<button type=button class=\"btn auto\" id=autoOn>An</button> <button type=button class=\"btn auto off\" id=autoOff>Aus</button> &rarr; <b id=autoStatus>–</b></p>"
    "<div class=row><span class=\"led led-r\"></span><button type=button class=\"btn minus\" data-c=r data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togR>Aus</button><button type=button class=\"btn plus\" data-c=r data-a=plus>+10%</button><span id=pctR class=pct>0%</span></div>"
    "<div class=row><span class=\"led led-g\"></span><button type=button class=\"btn minus\" data-c=g data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togG>Aus</button><button type=button class=\"btn plus\" data-c=g data-a=plus>+10%</button><span id=pctG class=pct>0%</span></div>"
    "<div class=row><span class=\"led led-b\"></span><button type=button class=\"btn minus\" data-c=b data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togB>Aus</button><button type=button class=\"btn plus\" data-c=b data-a=plus>+10%</button><span id=pctB class=pct>0%</span></div>"
    "<div class=row><span class=\"led led-w\"></span><button type=button class=\"btn minus\" data-c=w data-a=minus>−10%</button>"
    "<button type=button class=\"btn onoff\" id=togW>Aus</button><button type=button class=\"btn plus\" data-c=w data-a=plus>+10%</button><span id=pctW class=pct>0%</span></div>"
    "<p><button type=button class=\"btn alloff\" id=alloff>Alle LEDs aus</button></p>"
    "<p style=margin-top:1rem;><b>Animation (10 s)</b><br>"
    "<select id=animSelect style=padding:0.4rem;background:#333;color:#eee;border-radius:6px;margin-right:0.5rem;>"
    "<option value=1>Zufallsfade</option><option value=2>Regenbogen</option><option value=3>Weiß aufdrehen</option>"
    "<option value=4>Sternenfunkeln</option><option value=5>Geburtstag</option><option value=6>Nacht (Rot atmend)</option></select>"
    "<button type=button class=btn id=playAnim style=background:#2a5a2a;color:#9f9;>Go</button></p></main><script>"
    "function load(){ fetch('/api/state').then(function(r){return r.json();}).then(function(d){"
    "document.getElementById('autoStatus').textContent=d.auto? 'An':'Aus';"
    "var ch=['r','g','b','w'], id=['pctR','pctG','pctB','pctW'], tog=['togR','togG','togB','togW'];"
    "for(var i=0;i<4;i++){ document.getElementById(id[i]).textContent=d[ch[i]]+'%'; document.getElementById(tog[i]).textContent=d[ch[i]]>0?'An':'Aus'; }"
    "});}"
    "function post(url,body){ return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body||''}).then(load);}"
    "document.getElementById('autoOn').onclick=function(){ post('/api/auto','on=1'); };"
    "document.getElementById('autoOff').onclick=function(){ post('/api/auto','on=0'); };"
    "document.getElementById('alloff').onclick=function(){ post('/api/alloff'); };"
    "['minus','plus'].forEach(function(a){ var bt=document.querySelectorAll('[data-a='+a+']');"
    "for(var i=0;i<bt.length;i++){ (function(act,btn){ btn.onclick=function(){ var c=btn.getAttribute('data-c'); post('/api/color','c='+c+'&a='+act); }; })(a,bt[i]); }"
    "});"
    "['togR','togG','togB','togW'].forEach(function(id){ var c=id.replace('tog','').toLowerCase();"
    "document.getElementById(id).onclick=function(){ post('/api/color','c='+c+'&a=toggle'); }; });"
    "document.getElementById('playAnim').onclick=function(){ var v=document.getElementById('animSelect').value; post('/api/play','anim='+v); };"
    "load();</script></body></html>"
  ));
}

void handleApiState() {
  String json = "{\"auto\":";
  json += automationOn ? "1" : "0";
  json += ",\"r\":"; json += manual_r;
  json += ",\"g\":"; json += manual_g;
  json += ",\"b\":"; json += manual_b;
  json += ",\"w\":"; json += manual_w;
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
  if (!server.hasArg(F("c")) || !server.hasArg(F("a"))) { server.send(400, F("text/plain"), F("c und a fehlen")); return; }
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

// Animation für 10 s abspielen (1–5 wie unten, 6=Nacht rot atmend)
void handleApiPlay() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  if (!server.hasArg(F("anim"))) { server.send(400, F("text/plain"), F("anim=1..6")); return; }
  int anim = server.arg(F("anim")).toInt();
  if (anim < 1 || anim > 6) { server.send(400, F("text/plain"), F("anim 1..6")); return; }

  if (anim == 6) {
    unsigned long start = millis();
    while (millis() - start < 10000uL) {
      updateNightRed();
      handleNetwork();
      delay(100);
    }
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

// Prüft, ob das aktuelle Datum (lokal, NTP + TIMEZONE_OFFSET_SEC) ein in birthdays.h eingetragener Tag ist.
bool isBirthdayToday(long unixTimeUtc) {
  if (BIRTHDAY_COUNT == 0) return false;
  time_t t = (time_t)(unixTimeUtc + TIMEZONE_OFFSET_SEC);
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

// 0–7 Uhr Ortszeit: nur sanft animiertes Rot, max. 10 % Helligkeit
#define NIGHT_HOUR_START  0   // 0 Uhr
#define NIGHT_HOUR_END    7   // 7 Uhr (exklusive)
#define NIGHT_BRIGHTNESS_MAX  25   // 255 * 10% ≈ 25
#define NIGHT_BRIGHTNESS_MIN  5    // Untere Grenze, damit Rot nie ganz aus geht

bool isNightMode(long unixTimeUtc) {
  if (unixTimeUtc <= 0) return false;
  time_t t = (time_t)(unixTimeUtc + TIMEZONE_OFFSET_SEC);
  struct tm *tm = gmtime(&t);
  if (!tm) return false;
  int hour = tm->tm_hour;
  return (hour >= NIGHT_HOUR_START && hour < NIGHT_HOUR_END);
}

// Sanftes Rot-„Atmen“ (min → max → min), Zyklus ~10 s; geht nie ganz aus
void updateNightRed() {
  unsigned long phase = millis() % 10000uL;  // 10 s Zyklus
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

  // LEDs sofort ausschalten (bevor WiFi/OTA – sonst Strip-Zustand unbestimmt)
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
  // WiFi: SSID und Passwort aus .credentials (steht in .gitignore).
  // ========================================================================================
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  const unsigned long wifiTimeout = 10000;  // 10 s, danach ohne WiFi weitermachen (Testbed)
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
    Serial.println("Kein WiFi (Timeout). Fahre ohne Netzwerk fort – PIR/LED funktionieren.");
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

  
  // OTA: Du pushst neue Firmware von Cursor/PC aus (arduino-cli upload -p <ESP-IP>).
  // ESP holt nichts selbst von einem Server.
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.onStart([]() {
      Serial.printf("OTA Start – freier Heap: %u Byte\n", ESP.getFreeHeap());
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA Ende – warte 2 s vor Neustart (damit OK an PC geht).");
      delay(2000);  // Verhindert Timeout in espota: ESP muss OK senden, bevor er neu startet
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA Fortschritt: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA Fehler[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive");
      else if (error == OTA_END_ERROR) Serial.println("End");
    });
    ArduinoOTA.begin();
#if DEBUG
  Serial.println("OTA bereit. Upload von PC: arduino-cli upload -p " + WiFi.localIP().toString() + " ...");
#endif
  }

  // Web-Server: Frontend cachebar (GET /), API GET/POST
  server.on(F("/"), handleIndex);
  server.on(F("/index.html"), handleIndex);
  server.on(F("/api/state"), HTTP_GET, handleApiState);
  server.on(F("/api/auto"), HTTP_POST, handleApiAuto);
  server.on(F("/api/color"), HTTP_POST, handleApiColor);
  server.on(F("/api/alloff"), HTTP_POST, handleApiAlloff);
  server.on(F("/api/play"), HTTP_POST, handleApiPlay);
  server.onNotFound([]() { server.send(404, F("text/plain"), F("Not Found")); });
  server.begin();
#if DEBUG
  Serial.println("Web-Server: http://" + WiFi.localIP().toString());
#endif

  // Strip bereits am Anfang initialisiert; hier nur sicherstellen, dass aus
  setAll(0, 0, 0, 0);
  strip.show();

  // initialise the random generator
  randomSeed(ESP.getCycleCount());
  
  pinMode(PIR1, INPUT);
  pinMode(PIR2, INPUT);

#if DEBUG
  Serial.println();
  Serial.println("=== Setup fertig ===");
  Serial.println("LEDs aus. PIR1 = D0/GPIO16 (oben), PIR2 = D2/GPIO4 (unten).");
  Serial.println("Warte auf Bewegung... (PIR=HIGH = ausgelöst)");
  Serial.println("================================");
#endif

  // Boot-Signal: 3× grün aufblinken (1 s an, 1 s Pause), 50 % Helligkeit
  for (int b = 0; b < 3; b++) {
    setAll(0, 127, 0, 0);   // Grün, 50 % (127/255)
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
  int lastPir1 = -1, lastPir2 = -1;  // für Debug: Änderung erkennen
  static int lastAnimation = 0;      // 0 = noch keine, 1–4 = letzte gewählte Animation (keine Doppel)
  static bool wasNightMode = false;
  while (true) {
    handleNetwork();
    watchdogCount = 0;
    bool night = isNightMode(currenttime);

    // 0–7 Uhr: nur rot, sanft animiert, max. 10 % Helligkeit (kein PIR, kein Manual)
    if (night) {
      updateNightRed();
      if (!wasNightMode) wasNightMode = true;
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
    // Alle ~2 s: PIR-Zustand ausgeben (Heartbeat)
    if (count % 20 == 0) {
      Serial.printf("[%lu] PIR1=%s PIR2=%s  (PIR1=UP, PIR2=DOWN)\n",
                    millis() / 1000, pir1 == HIGH ? "HIGH" : "low ", pir2 == HIGH ? "HIGH" : "low ");
    }
    // Bei Zustandsänderung sofort melden
    if (pir1 != lastPir1 || pir2 != lastPir2) {
      Serial.printf("      PIR geändert -> PIR1=%s PIR2=%s\n", pir1 == HIGH ? "HIGH" : "low ", pir2 == HIGH ? "HIGH" : "low ");
      lastPir1 = pir1;
      lastPir2 = pir2;
    }
#endif

    if (pir1 == HIGH) { dir = "UP"; }
    if (pir2 == HIGH) { dir = "DOWN"; }

    // PIR-Animation nur außerhalb des Nachtmodus
    if (!night && automationOn && dir != "") {
#if DEBUG
      Serial.println(">>> PIR ausgelöst: " + dir + " -> starte Animation");
#endif
      if (isBirthdayToday(currenttime)) {
        birthday(dir);
        // 10 s Pause nach Geburtstags-Animation (wie bei den anderen)
        for (unsigned long t = millis(); millis() - t < (unsigned long)POST_ANIM_DELAY_MS; ) {
          handleNetwork();
          delay(100);
        }
      } else {
        // Zufall 1–4, aber nicht dieselbe Animation wie beim letzten Mal
        int choice;
        do {
          choice = random(1, 5);
        } while (choice == lastAnimation);
        lastAnimation = choice;

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
        // 10 s Pause nach Animation (OTA und Web bleiben erreichbar)
        for (unsigned long t = millis(); millis() - t < (unsigned long)POST_ANIM_DELAY_MS; ) {
          handleNetwork();
          delay(100);
        }
      }
    }
    dir = "";

    if (count++ > 100) {
      currenttime = ntpClient.getUnixTime();
#if DEBUG
      Serial.printf("NTP Zeit: %d\n", currenttime);
#endif
      count = 0;
    }
    yield();
    delay(100);
  }
}

