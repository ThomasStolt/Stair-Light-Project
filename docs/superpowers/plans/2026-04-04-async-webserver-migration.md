# AsyncWebServer Migration & Web UI Redesign — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate from ESP8266WebServer to ESPAsyncWebServer for non-blocking request handling, then redesign the web UI with sliders, animation buttons, collapsible diagnostics, and tiered polling.

**Architecture:** Two-phase incremental approach. Phase 1 (Tasks 1–3) swaps the server library while keeping the existing UI identical, so any breakage is clearly a library issue. Phase 2 (Tasks 4–6) redesigns the UI HTML/JS/CSS. All HTML stays embedded in PROGMEM.

**Tech Stack:** ESP8266 Arduino, ESPAsyncTCP, ESPAsyncWebServer, Adafruit NeoPixel, ArduinoOTA, EasyNTPClient

---

## File Structure

| File | Action | Responsibility |
|------|--------|---------------|
| `rgbw_stair_light/rgbw_stair_light.ino` | Modify | Server setup, handlers, HTML, loop logic |
| `rgbw_stair_light/parking.h` | Modify | Remove all `handleNetwork()` calls from animations |

No new files are created.

---

## Phase 1: AsyncWebServer Library Swap

### Task 1: Replace includes, server declaration, and remove handleNetwork()

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:27-41` (includes)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:92` (server declaration)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:98-99` (add pending anim/reboot globals)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:163-171` (handleNetwork declaration and definition)
- Modify: `rgbw_stair_light/parking.h:1` (remove ArduinoOTA include)
- Modify: `rgbw_stair_light/parking.h` (remove all handleNetwork() calls — 17 occurrences)

- [ ] **Step 1: Replace the include and server declaration in the .ino**

In `rgbw_stair_light/rgbw_stair_light.ino`, replace:
```cpp
#include <ESP8266WebServer.h>
```
with:
```cpp
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
```

Replace:
```cpp
ESP8266WebServer server(80);
```
with:
```cpp
AsyncWebServer server(80);
```

- [ ] **Step 2: Add pending animation and reboot flag globals**

In `rgbw_stair_light/rgbw_stair_light.ino`, after line 99 (`uint32_t g_animDurationOverrideMs = 0;`), add:
```cpp
// Flags set by async web handlers, consumed by loop()
volatile int g_pendingPlayAnim = 0;   // 0 = none, 1-6 = animation to play
volatile bool g_pendingReboot = false;
```

- [ ] **Step 3: Remove handleNetwork() declaration and definition**

In `rgbw_stair_light/rgbw_stair_light.ino`, delete lines 163-164:
```cpp
// Called in animations and wait loops so OTA and web server stay responsive during animation
void handleNetwork(void);
```

Delete lines 168-171:
```cpp
void handleNetwork(void) {
  ArduinoOTA.handle();
  server.handleClient();
}
```

- [ ] **Step 4: Remove handleNetwork() calls from parking.h**

In `rgbw_stair_light/parking.h`, delete line 1:
```cpp
#include <ArduinoOTA.h>
```
(ArduinoOTA is already included in the .ino, and parking.h no longer calls handleNetwork which called ArduinoOTA.handle().)

Remove every `handleNetwork();` call in parking.h. These are on lines: 120, 157, 183, 197, 228, 242, 281, 295, 358, 370, 385, 429, 474, 480, 499. Delete each line entirely (they are standalone statements, not part of larger expressions).

- [ ] **Step 5: Remove handleNetwork() calls from the .ino loop**

In `rgbw_stair_light/rgbw_stair_light.ino`, replace every `handleNetwork();` in the loop and wait loops with `ArduinoOTA.handle();`. These are on lines: 732, 778, 790, 818.

The main loop still needs `ArduinoOTA.handle()` since OTA uses a different mechanism than the web server. The web server no longer needs pumping (async handles it).

- [ ] **Step 6: Compile check**

Run:
```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 rgbw_stair_light/
```

Expected: compilation errors for handler functions (they still use `server.send()` etc.) — that is fine, we fix those in Task 2. But the handleNetwork removal and includes should be clean.

If ESPAsyncTCP or ESPAsyncWebServer libraries are not installed:
```bash
arduino-cli lib install "ESPAsyncTCP"
arduino-cli lib install "ESP Async WebServer"
```

- [ ] **Step 7: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino rgbw_stair_light/parking.h
git commit -m "Phase 1a: Replace ESP8266WebServer with ESPAsyncWebServer, remove handleNetwork()"
```

---

### Task 2: Convert all request handlers to async API

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:207-295` (handleIndex)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:297-308` (handleApiState)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:310-318` (handleApiAuto)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:320-356` (handleApiColor)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:358-364` (handleApiAlloff)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:367-391` (handleApiPlay)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:393-432` (handleApiTime)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:435-442` (handleApiReboot)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:444-470` (handleApiLog)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:476-521` (handleApiMemory)
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:539-558` (handleApiSysinfo)

- [ ] **Step 1: Convert handleIndex**

Replace the function with:
```cpp
void handleIndex(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", PSTR(
    // ... existing HTML string unchanged ...
  ));
  response->addHeader(F("Cache-Control"), F("public, max-age=3600"));
  request->send(response);
}
```

Note: `beginResponse_P` reads from PROGMEM. The HTML content string stays exactly the same as the current version. Keep all the existing HTML between `PSTR(` and `)` identical.

- [ ] **Step 2: Convert handleApiState**

Replace:
```cpp
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
```
with:
```cpp
void handleApiState(AsyncWebServerRequest *request) {
  String json = "{\"auto\":";
  json += automationOn ? "1" : "0";
  json += ",\"r\":"; json += manual_r;
  json += ",\"g\":"; json += manual_g;
  json += ",\"b\":"; json += manual_b;
  json += ",\"w\":"; json += manual_w;
  json += ",\"night\":"; json += isNightMode(g_lastNtpTime) ? "1" : "0";
  json += "}";
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}
```

- [ ] **Step 3: Convert handleApiAuto**

Replace:
```cpp
void handleApiAuto() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  if (server.hasArg(F("on"))) {
    automationOn = (server.arg(F("on")).toInt() != 0);
    if (!automationOn) applyManualColor();
    else { setAll(0, 0, 0, 0); strip.show(); }
  }
  server.send(204);
}
```
with:
```cpp
void handleApiAuto(AsyncWebServerRequest *request) {
  if (request->hasParam(F("on"), true)) {
    automationOn = (request->getParam(F("on"), true)->value().toInt() != 0);
    if (!automationOn) applyManualColor();
    else { setAll(0, 0, 0, 0); strip.show(); }
  }
  request->send(204);
}
```

Note: The `HTTP_POST` method check is removed because the route is registered with `HTTP_POST` — only POST requests reach this handler. The second parameter `true` in `hasParam`/`getParam` means "search POST body parameters".

- [ ] **Step 4: Convert handleApiColor (with new v= parameter)**

Replace the entire `handleApiColor` function with:
```cpp
void handleApiColor(AsyncWebServerRequest *request) {
  if (request->hasParam(F("all"), true)) {
    int pct;
    if (request->hasParam(F("a"), true)) {
      String ax = request->getParam(F("a"), true)->value();
      int cur = (int)manual_r;
      if (ax == F("minus10")) pct = cur - 10;
      else if (ax == F("minus1")) pct = cur - 1;
      else if (ax == F("plus1")) pct = cur + 1;
      else if (ax == F("plus10")) pct = cur + 10;
      else { request->send(400, F("text/plain"), F("a=minus10|minus1|plus1|plus10")); return; }
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
    } else {
      pct = request->getParam(F("all"), true)->value().toInt();
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
    }
    manual_r = manual_g = manual_b = manual_w = (uint8_t)pct;
    if (!automationOn) applyManualColor();
    request->send(204);
    return;
  }
  // Direct value set: c=<channel>&v=<0-100> (used by sliders in Phase 2)
  if (request->hasParam(F("c"), true) && request->hasParam(F("v"), true)) {
    String c = request->getParam(F("c"), true)->value();
    int val = request->getParam(F("v"), true)->value().toInt();
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    uint8_t* target = nullptr;
    if (c == F("r")) target = &manual_r;
    else if (c == F("g")) target = &manual_g;
    else if (c == F("b")) target = &manual_b;
    else if (c == F("w")) target = &manual_w;
    if (!target) { request->send(400, F("text/plain"), F("c=r|g|b|w")); return; }
    *target = (uint8_t)val;
    if (!automationOn) applyManualColor();
    request->send(204);
    return;
  }
  // Step adjust: c=<channel>&a=minus|plus|toggle
  if (!request->hasParam(F("c"), true) || !request->hasParam(F("a"), true)) {
    request->send(400, F("text/plain"), F("c and a (or v) required"));
    return;
  }
  String c = request->getParam(F("c"), true)->value();
  String a = request->getParam(F("a"), true)->value();
  uint8_t* v = nullptr;
  if (c == F("r")) v = &manual_r; else if (c == F("g")) v = &manual_g; else if (c == F("b")) v = &manual_b; else if (c == F("w")) v = &manual_w;
  if (!v) { request->send(400, F("text/plain"), F("c=r|g|b|w")); return; }
  if (a == F("minus")) { *v = (*v <= 10) ? 0 : (*v - 10); }
  else if (a == F("plus"))  { *v = (*v >= 90) ? 100 : (*v + 10); }
  else if (a == F("toggle")) { *v = (*v > 0) ? 0 : 50; }
  if (!automationOn) applyManualColor();
  request->send(204);
}
```

Note: This already includes the `v=<value>` direct-set parameter needed for Phase 2 sliders. Adding it now is harmless and saves a second touch of this handler later.

- [ ] **Step 5: Convert handleApiAlloff**

Replace:
```cpp
void handleApiAlloff() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  manual_r = manual_g = manual_b = manual_w = 0;
  setAll(0, 0, 0, 0);
  strip.show();
  server.send(204);
}
```
with:
```cpp
void handleApiAlloff(AsyncWebServerRequest *request) {
  manual_r = manual_g = manual_b = manual_w = 0;
  setAll(0, 0, 0, 0);
  strip.show();
  request->send(204);
}
```

- [ ] **Step 6: Convert handleApiPlay to flag-based**

Replace the entire function:
```cpp
void handleApiPlay(AsyncWebServerRequest *request) {
  if (!request->hasParam(F("anim"), true)) { request->send(400, F("text/plain"), F("anim=1..6")); return; }
  int anim = request->getParam(F("anim"), true)->value().toInt();
  if (anim < 1 || anim > 6) { request->send(400, F("text/plain"), F("anim 1..6")); return; }
  g_pendingPlayAnim = anim;
  request->send(204);
}
```

- [ ] **Step 7: Convert handleApiTime**

Replace the entire function with:
```cpp
void handleApiTime(AsyncWebServerRequest *request) {
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
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}
```

- [ ] **Step 8: Convert handleApiReboot to flag-based**

Replace:
```cpp
void handleApiReboot() {
  if (server.method() != HTTP_POST) { server.send(405, F("text/plain"), F("Method Not Allowed")); return; }
  server.send(204);
  server.handleClient();
  delay(300);
  ESP.restart();
}
```
with:
```cpp
void handleApiReboot(AsyncWebServerRequest *request) {
  g_pendingReboot = true;
  request->send(204);
}
```

- [ ] **Step 9: Convert handleApiLog**

Replace the entire function with:
```cpp
void handleApiLog(AsyncWebServerRequest *request) {
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
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}
```

- [ ] **Step 10: Convert handleApiMemory**

Replace the entire function with:
```cpp
#ifndef ESP8266_HEAP_TOTAL
#define ESP8266_HEAP_TOTAL 80192u
#endif
void handleApiMemory(AsyncWebServerRequest *request) {
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
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}
```

- [ ] **Step 11: Convert handleApiSysinfo**

Replace the entire function with:
```cpp
void handleApiSysinfo(AsyncWebServerRequest *request) {
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
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}
```

- [ ] **Step 12: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Phase 1b: Convert all request handlers to AsyncWebServer API"
```

---

### Task 3: Update route registration, loop() logic, and verify compilation

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:676-689` (route registration in setup())
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:723-837` (loop() — add pending anim/reboot checks)

- [ ] **Step 1: Update route registration in setup()**

Replace:
```cpp
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
```
with:
```cpp
  // Web server: frontend cacheable (GET /), API GET/POST (AsyncWebServer)
  server.on("/", HTTP_GET, handleIndex);
  server.on("/index.html", HTTP_GET, handleIndex);
  server.on("/api/state", HTTP_GET, handleApiState);
  server.on("/api/auto", HTTP_POST, handleApiAuto);
  server.on("/api/color", HTTP_POST, handleApiColor);
  server.on("/api/alloff", HTTP_POST, handleApiAlloff);
  server.on("/api/play", HTTP_POST, handleApiPlay);
  server.on("/api/time", HTTP_GET, handleApiTime);
  server.on("/api/reboot", HTTP_POST, handleApiReboot);
  server.on("/api/log", HTTP_GET, handleApiLog);
  server.on("/api/memory", HTTP_GET, handleApiMemory);
  server.on("/api/sysinfo", HTTP_GET, handleApiSysinfo);
  server.onNotFound([](AsyncWebServerRequest *request) { request->send(404, F("text/plain"), F("Not Found")); });
  server.begin();
```

Note: `AsyncWebServer::on()` takes plain `const char*` for paths (not `F()` macro). Method parameter is required for all routes.

- [ ] **Step 2: Add pending animation and reboot handling to loop()**

In the main `while (true)` loop in `loop()`, right after `ArduinoOTA.handle();` and `watchdogCount = 0;`, add:

```cpp
    // Handle pending reboot from web UI
    if (g_pendingReboot) {
      delay(300);
      ESP.restart();
    }

    // Handle pending animation from web UI
    if (g_pendingPlayAnim > 0) {
      int anim = g_pendingPlayAnim;
      g_pendingPlayAnim = 0;
      g_animDurationOverrideMs = 10000;
      if (anim == 6) {
        nightAnimation("UP");
        setAll(0, 0, 0, 0);
        strip.show();
      } else if (anim == 1) simpleFadeToRandom(F("UP"));
      else if (anim == 2) rainbowSteps(F("UP"));
      else if (anim == 3) FadeToFullBrightness(F("UP"));
      else if (anim == 4) starSparkle(F("UP"));
      else if (anim == 5) birthday(F("UP"));
      g_animDurationOverrideMs = 0;
      if (!automationOn) applyManualColor();
      else { setAll(0, 0, 0, 0); strip.show(); }
    }
```

- [ ] **Step 3: Compile**

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 rgbw_stair_light/
```

Expected: clean compilation with no errors.

If there are errors, most likely causes:
- Missing `true` parameter in `hasParam`/`getParam` for POST body params
- Stray `server.` references not yet converted (search for `server.send`, `server.hasArg`, `server.arg`)

- [ ] **Step 4: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Phase 1c: Update route registration and loop() for async handlers"
```

- [ ] **Step 5: Upload to ESP and test**

```bash
cd /Users/thomasstolt/Library/CloudStorage/OneDrive-Persoenlich/Documents/Github/Stair-Light-Project
./upload-to-esp8266-ota.sh
```

Verify all endpoints work:
- Open `http://<ESP-IP>/` — existing UI loads
- Automation On/Off buttons work
- RGBW +/- buttons work
- All-channel presets work
- Animation dropdown + Go plays the animation
- Reboot button reboots
- Motion log, memory, CPU, WiFi tables populate
- OTA upload works after reboot

- [ ] **Step 6: Commit any fixes**

If any fixes were needed:
```bash
git add rgbw_stair_light/rgbw_stair_light.ino rgbw_stair_light/parking.h
git commit -m "Phase 1: Fix issues found during hardware testing"
```

---

## Phase 2: Web UI Redesign

### Task 4: Rewrite the HTML/CSS in handleIndex

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino:209-293` (the PROGMEM HTML string in handleIndex)

- [ ] **Step 1: Replace the entire HTML string**

Replace the HTML PSTR string inside `handleIndex` (everything between `beginResponse_P(200, "text/html", PSTR(` and the closing `));`). The full replacement — note this does NOT include the `<script>` tag yet (that is Task 5):

```cpp
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", PSTR(
    "<!DOCTYPE html><html><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>Stair Light</title><style>"
    "body{font-family:sans-serif;margin:0;background:#1a1a1a;color:#eee;display:flex;flex-direction:column;align-items:center;min-height:100vh;padding:1.5rem 1rem;box-sizing:border-box;}"
    "main{text-align:center;max-width:420px;width:100%;} h1{font-size:1.3rem;margin:0 0 1rem;}"
    ".section{background:#222;padding:0.8rem;border-radius:8px;margin-bottom:0.8rem;}"
    "button.btn{padding:0.7rem 1rem;border-radius:8px;font-size:1rem;cursor:pointer;border:1px solid rgba(0,0,0,0.25);min-height:2.4em;"
    "box-shadow:inset 0 1px 0 rgba(255,255,255,0.15),0 2px 4px rgba(0,0,0,0.3);text-shadow:0 1px 1px rgba(0,0,0,0.3);transition:box-shadow .1s,transform .1s;}"
    "button.btn:active{box-shadow:inset 0 2px 6px rgba(0,0,0,0.4);transform:translateY(1px);}"
    ".auto-on{background:linear-gradient(180deg,#3a5a3a,#1a3a1a);color:#9f9;}"
    ".auto-off{background:linear-gradient(180deg,#5a3a3a,#3a1a1a);color:#f99;}"
    ".preset{background:linear-gradient(180deg,#3a4a5c,#1a2a38);color:#8cf;padding:0.5rem 0.7rem;font-size:0.95rem;}"
    ".reboot{background:linear-gradient(180deg,#884422,#442200);color:#ffc;}"
    ".anim-btn{padding:0.6rem 0.8rem;font-size:0.9rem;border-radius:8px;cursor:pointer;border:1px solid rgba(0,0,0,0.25);"
    "box-shadow:inset 0 1px 0 rgba(255,255,255,0.15),0 2px 4px rgba(0,0,0,0.3);transition:box-shadow .1s,transform .1s;}"
    ".anim-btn:active{box-shadow:inset 0 2px 6px rgba(0,0,0,0.4);transform:translateY(1px);}"
    ".a1{background:linear-gradient(180deg,#3a5a3a,#1a3a1a);color:#9f9;}"
    ".a2{background:linear-gradient(180deg,#2a3a5a,#1a2a4a);color:#8cf;}"
    ".a3{background:linear-gradient(180deg,#4a4a4a,#2a2a2a);color:#eee;}"
    ".a4{background:linear-gradient(180deg,#1a2a4a,#0a1a3a);color:#88f;}"
    ".a5{background:linear-gradient(180deg,#5a3a5a,#3a1a3a);color:#f8f;}"
    ".a6{background:linear-gradient(180deg,#5a2a2a,#3a1a1a);color:#f88;}"
    ".led{width:12px;height:12px;border-radius:50%;display:inline-block;flex-shrink:0;box-shadow:0 0 6px currentColor;}"
    ".led-r{background:#e00;color:#e00;} .led-g{background:#0a0;color:#0a0;} .led-b{background:#06f;color:#06f;} .led-w{background:#eee;color:#eee;}"
    ".slider-row{display:flex;align-items:center;gap:8px;margin:6px 0;}"
    "input[type=range]{flex:1;height:6px;-webkit-appearance:none;appearance:none;background:#333;border-radius:3px;outline:none;}"
    "input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;border-radius:50%;cursor:pointer;border:2px solid #555;}"
    ".sr input::-webkit-slider-thumb{background:#e00;} .sg input::-webkit-slider-thumb{background:#0a0;} .sb input::-webkit-slider-thumb{background:#06f;} .sw input::-webkit-slider-thumb{background:#eee;}"
    "span.pct{min-width:2.5em;text-align:right;font-size:0.9rem;}"
    ".row{display:flex;align-items:center;justify-content:center;gap:6px;margin:6px 0;flex-wrap:wrap;}"
    "table{width:100%;border-collapse:collapse;font-size:0.85rem;margin-top:0.3rem;} th,td{border:1px solid #444;padding:0.25rem 0.4rem;text-align:left;} th{background:#333;}"
    "details{background:#222;border-radius:8px;margin-bottom:0.5rem;} summary{padding:0.6rem 0.8rem;cursor:pointer;font-weight:bold;font-size:0.95rem;color:#aaa;}"
    "details[open] summary{color:#eee;} details .inner{padding:0 0.8rem 0.8rem;}"
    "</style></head><body><main>"
    "<h1>Stair Light</h1>"
    "<p style=margin-bottom:0.5rem;font-size:0.95rem;><span id=dateDisplay>--</span> <span id=timeDisplay>--</span></p>"
    "<p style=margin-bottom:0.3rem;font-size:0.85rem;>Uptime: <span id=uptimeDisplay>--</span> &nbsp; Last reboot: <span id=lastRebootDisplay>--</span></p>"
    "<div class=section>"
    "<b>Stair automation</b><br>"
    "<div class=row><button type=button class=\"btn auto-on\" id=autoOn>On</button>"
    "<button type=button class=\"btn auto-off\" id=autoOff>Off</button>"
    "<b id=autoStatus style=margin-left:0.3rem;>&ndash;</b></div>"
    "<p id=nightBadge style=\"display:none;margin:0.4rem 0 0;padding:0.4rem 0.8rem;border-radius:8px;background:rgba(180,40,40,0.25);border:1px solid #a33;font-size:0.9rem;color:#f88;\">Night mode active (" NIGHT_HOUR_START_STR ":00 &ndash; " NIGHT_HOUR_END_STR ":00)</p>"
    "</div>"
    "<div class=section>"
    "<b>RGBW Channels</b>"
    "<div class=\"slider-row sr\"><span class=\"led led-r\"></span><input type=range id=slR min=0 max=100 value=0><span id=pctR class=pct>0%</span></div>"
    "<div class=\"slider-row sg\"><span class=\"led led-g\"></span><input type=range id=slG min=0 max=100 value=0><span id=pctG class=pct>0%</span></div>"
    "<div class=\"slider-row sb\"><span class=\"led led-b\"></span><input type=range id=slB min=0 max=100 value=0><span id=pctB class=pct>0%</span></div>"
    "<div class=\"slider-row sw\"><span class=\"led led-w\"></span><input type=range id=slW min=0 max=100 value=0><span id=pctW class=pct>0%</span></div>"
    "<div class=row style=margin-top:0.5rem;><b style=margin-right:0.3rem;>All</b>"
    "<button type=button class=\"btn preset\" data-all=0>0%</button>"
    "<button type=button class=\"btn preset\" data-all=25>25%</button>"
    "<button type=button class=\"btn preset\" data-all=50>50%</button>"
    "<button type=button class=\"btn preset\" data-all=75>75%</button>"
    "<button type=button class=\"btn preset\" data-all=100>100%</button></div>"
    "</div>"
    "<div class=section>"
    "<b>Animations (10 s)</b>"
    "<div class=row style=margin-top:0.5rem;>"
    "<button type=button class=\"anim-btn a1\" data-anim=1>Random fade</button>"
    "<button type=button class=\"anim-btn a2\" data-anim=2>Rainbow</button>"
    "<button type=button class=\"anim-btn a3\" data-anim=3>White ramp</button>"
    "</div><div class=row>"
    "<button type=button class=\"anim-btn a4\" data-anim=4>Star sparkle</button>"
    "<button type=button class=\"anim-btn a5\" data-anim=5>Birthday</button>"
    "<button type=button class=\"anim-btn a6\" data-anim=6>Night red</button>"
    "</div></div>"
    "<p><button type=button class=\"btn reboot\" id=rebootBtn>Reboot</button></p>"
    "<details id=detailLog><summary>Last 5 motions</summary><div class=inner>"
    "<table><thead><tr><th>Time</th><th>Direction</th><th>Animation</th></tr></thead><tbody id=motionLogBody></tbody></table></div></details>"
    "<details id=detailMem><summary>Memory status</summary><div class=inner>"
    "<table><thead><tr><th>Type</th><th>Total</th><th>Used</th><th>Usage</th></tr></thead><tbody id=memoryBody></tbody></table></div></details>"
    "<details id=detailCpu><summary>CPU / Runtime</summary><div class=inner>"
    "<table><thead><tr><th>Key</th><th>Value</th></tr></thead><tbody id=cpuBody></tbody></table></div></details>"
    "<details id=detailWifi><summary>WiFi</summary><div class=inner>"
    "<table><thead><tr><th>Key</th><th>Value</th></tr></thead><tbody id=wifiBody></tbody></table></div></details>"
    "<p style=margin-top:1rem;font-size:0.75rem;color:#666;>Firmware v" FW_VERSION "</p></main>"
    "<script>"
    "/* JS added in Task 5 */"
    "</script></body></html>"
  ));
  response->addHeader(F("Cache-Control"), F("public, max-age=3600"));
  request->send(response);
```

- [ ] **Step 2: Compile to verify HTML syntax**

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 rgbw_stair_light/
```

Expected: clean compilation. If errors, check for unescaped quotes or missing string concatenation.

- [ ] **Step 3: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Phase 2a: Rewrite HTML with sliders, animation buttons, collapsible diagnostics"
```

---

### Task 5: Rewrite the JavaScript (polling, sliders, event handlers)

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (the `<script>` section inside the PROGMEM HTML)

- [ ] **Step 1: Replace the placeholder script with the full JavaScript**

Replace `"/* JS added in Task 5 */"` with the full JavaScript. The complete `<script>` block (replace the entire `"<script>"..."</script></body></html>"` lines):

```cpp
    "<script>"
    "var sliders={r:document.getElementById('slR'),g:document.getElementById('slG'),b:document.getElementById('slB'),w:document.getElementById('slW')};"
    "var pcts={r:document.getElementById('pctR'),g:document.getElementById('pctG'),b:document.getElementById('pctB'),w:document.getElementById('pctW')};"
    "var debounce={};"
    "function post(url,body){return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body||''});}"
    "function loadFast(){"
    "Promise.all([fetch('/api/state').then(function(r){return r.json();}),fetch('/api/time').then(function(r){return r.json();})]).then(function(arr){"
    "var d=arr[0],t=arr[1];"
    "document.getElementById('autoStatus').textContent=d.auto?'On':'Off';"
    "document.getElementById('nightBadge').style.display=d.night?'block':'none';"
    "var ch=['r','g','b','w'];"
    "for(var i=0;i<4;i++){if(!debounce[ch[i]]){sliders[ch[i]].value=d[ch[i]];pcts[ch[i]].textContent=d[ch[i]]+'%';}}"
    "document.getElementById('dateDisplay').textContent=t.date||'--';"
    "document.getElementById('timeDisplay').textContent=t.time||'--';"
    "var ms=t.uptime_ms||0,dy=Math.floor(ms/86400000),h=Math.floor((ms%86400000)/3600000),m=Math.floor((ms%3600000)/60000);"
    "document.getElementById('uptimeDisplay').textContent=(dy>0?dy+'d ':'')+h+'h '+m+'m';"
    "document.getElementById('lastRebootDisplay').textContent=t.last_reboot||'--';"
    "});}"
    "function loadSlow(){"
    "var dl=document.getElementById('detailLog'),dm=document.getElementById('detailMem'),dc=document.getElementById('detailCpu'),dw=document.getElementById('detailWifi');"
    "var fetches=[],keys=[];"
    "if(dl.open){fetches.push(fetch('/api/log').then(function(r){return r.json();}));keys.push('log');}else{fetches.push(Promise.resolve(null));keys.push('log');}"
    "if(dm.open){fetches.push(fetch('/api/memory').then(function(r){return r.json();}));keys.push('mem');}else{fetches.push(Promise.resolve(null));keys.push('mem');}"
    "if(dc.open||dw.open){fetches.push(fetch('/api/sysinfo').then(function(r){return r.json();}));keys.push('sys');}else{fetches.push(Promise.resolve(null));keys.push('sys');}"
    "Promise.all(fetches).then(function(arr){"
    "var log=arr[0],mem=arr[1],sys=arr[2];"
    "if(log){"
    "var tb=document.getElementById('motionLogBody');tb.innerHTML='';"
    "for(var i=0;i<log.length;i++){var r=document.createElement('tr');r.innerHTML='<td>'+log[i].time+'</td><td>'+log[i].dir+'</td><td>'+log[i].anim+'</td>';tb.appendChild(r);}}"
    "if(mem){"
    "function fmt(n){return n>=1024?(n/1024).toFixed(1)+' KB':n+' B';}"
    "document.getElementById('memoryBody').innerHTML='<tr><td>Heap (RAM)</td><td>'+fmt(mem.heap_total)+'</td><td>'+fmt(mem.heap_used)+'</td><td>'+mem.heap_pct+'%</td></tr>"
    "<tr><td>Flash</td><td>'+fmt(mem.flash_size)+'</td><td>'+fmt(mem.flash_used)+'</td><td>'+mem.flash_pct+'%</td></tr>"
    "<tr><td>RTC</td><td>'+fmt(mem.rtc_total)+'</td><td>'+fmt(mem.rtc_used)+'</td><td>'+mem.rtc_pct+'%</td></tr>';}"
    "if(sys){"
    "var rsn=sys.reset_reason||'--';var exRow=(rsn.indexOf('xception')>=0)?'<tr><td>Exception info</td><td>'+sys.reset_info+'</td></tr>':'';"
    "document.getElementById('cpuBody').innerHTML='<tr><td>CPU freq</td><td>'+sys.cpu_mhz+' MHz</td></tr><tr><td>Reset reason</td><td>'+rsn+'</td></tr>'+exRow;"
    "function rssiQ(v){if(v>=-50)return 'Excellent';if(v>=-60)return 'Good';if(v>=-70)return 'Fair';return 'Poor';}"
    "var rssi=sys.rssi||0;"
    "document.getElementById('wifiBody').innerHTML='<tr><td>SSID</td><td>'+sys.ssid+'</td></tr><tr><td>BSSID</td><td>'+sys.bssid+'</td></tr><tr><td>IP</td><td>'+sys.ip+'</td></tr>"
    "<tr><td>Gateway</td><td>'+sys.gateway+'</td></tr><tr><td>DNS</td><td>'+sys.dns+'</td></tr><tr><td>Channel</td><td>'+sys.channel+'</td></tr>"
    "<tr><td>RSSI</td><td>'+rssi+' dBm ('+rssiQ(rssi)+')</td></tr><tr><td>Reconnects</td><td>'+sys.reconnects+'</td></tr>';}"
    "});}"
    "['r','g','b','w'].forEach(function(ch){"
    "sliders[ch].addEventListener('input',function(){"
    "pcts[ch].textContent=this.value+'%';"
    "debounce[ch]=true;clearTimeout(debounce[ch+'t']);"
    "debounce[ch+'t']=setTimeout(function(){post('/api/color','c='+ch+'&v='+sliders[ch].value).then(function(){debounce[ch]=false;});},100);"
    "});"
    "});"
    "document.getElementById('autoOn').onclick=function(){post('/api/auto','on=1').then(loadFast);};"
    "document.getElementById('autoOff').onclick=function(){post('/api/auto','on=0').then(loadFast);};"
    "document.querySelectorAll('.preset').forEach(function(btn){btn.onclick=function(){post('/api/color','all='+btn.getAttribute('data-all')).then(loadFast);};});"
    "document.querySelectorAll('.anim-btn').forEach(function(btn){btn.onclick=function(){post('/api/play','anim='+btn.getAttribute('data-anim'));};});"
    "document.getElementById('rebootBtn').onclick=function(){this.disabled=true;this.textContent='Rebooting...';post('/api/reboot').then(function(){setTimeout(function(){location.reload();},4000);});};"
    "loadFast();loadSlow();"
    "setInterval(loadFast,2000);"
    "setInterval(loadSlow,15000);"
    "</script></body></html>"
```

Key behaviors:
- `loadFast()` polls `/api/state` + `/api/time` every 2s
- `loadSlow()` polls `/api/log`, `/api/memory`, `/api/sysinfo` every 15s, but only when their `<details>` section is open (checks `.open` property)
- Slider `input` events are debounced at 100ms and use `POST /api/color` with `c=<channel>&v=<value>`
- During active slider dragging (`debounce[ch]` is true), `loadFast` skips updating that slider to prevent fighting with user input
- Preset buttons call `POST /api/color` with `all=<value>` then refresh via `loadFast`
- Animation buttons call `POST /api/play` with `anim=<1-6>`

- [ ] **Step 2: Compile**

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 rgbw_stair_light/
```

Expected: clean compilation.

- [ ] **Step 3: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Phase 2b: Add JavaScript for sliders, tiered polling, animation buttons"
```

---

### Task 6: Upload, test, and finalize

**Files:**
- Possibly modify: `rgbw_stair_light/rgbw_stair_light.ino` (bug fixes found during testing)

- [ ] **Step 1: Upload to ESP**

```bash
cd /Users/thomasstolt/Library/CloudStorage/OneDrive-Persoenlich/Documents/Github/Stair-Light-Project
./upload-to-esp8266-ota.sh
```

- [ ] **Step 2: Test the UI**

Open `http://<ESP-IP>/` in a mobile browser. Verify:

1. **Header**: date, time, uptime, last reboot display correctly
2. **Automation**: On/Off buttons toggle automation, status text updates
3. **Night mode badge**: shows/hides correctly based on time
4. **RGBW sliders**: dragging updates percentage label in real-time, LEDs respond
5. **Preset buttons**: 0/25/50/75/100% set all sliders and LEDs
6. **Animation buttons**: each triggers its animation for 10s
7. **Reboot button**: shows "Rebooting...", page reloads after ~4s
8. **Collapsible diagnostics**: all four sections expand/collapse, data loads when opened
9. **Polling**: state/time update every ~2s, diagnostics only when expanded
10. **PIR triggers**: walk past sensors, verify automation still works, motion log updates

- [ ] **Step 3: Update firmware version**

In `rgbw_stair_light/rgbw_stair_light.ino`, update:
```cpp
#define FW_VERSION "1.0.0"
```
to:
```cpp
#define FW_VERSION "2.0.0"
```

- [ ] **Step 4: Final commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Phase 2: Complete UI redesign with AsyncWebServer, bump to v2.0.0"
```
