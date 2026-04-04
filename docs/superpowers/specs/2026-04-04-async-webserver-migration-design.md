# AsyncWebServer Migration & Web UI Redesign

**Date:** 2026-04-04
**Status:** Approved

## Goal

Migrate from `ESP8266WebServer` (synchronous) to `ESPAsyncWebServer` (async) for improved web UI responsiveness, and redesign the web UI with sliders, individual animation buttons, collapsible diagnostics, and tiered polling.

## Approach: Incremental (Two Phases)

Phase 1 swaps the server library while keeping the existing UI identical. Phase 2 redesigns the UI. This separation ensures that if something breaks, the cause is obvious — library vs. UI change.

## Constraints

- ESP8266 (NodeMCU), ~37 kB free heap available
- HTML stays embedded in PROGMEM (no LittleFS) — single OTA binary deploy
- OTA must remain functional throughout
- All existing API response formats preserved

---

## Phase 1: AsyncWebServer Library Swap

### Dependencies

Replace:
- `#include <ESP8266WebServer.h>`

With:
- `#include <ESPAsyncTCP.h>`
- `#include <ESPAsyncWebServer.h>`

### Server Declaration

```cpp
// Before
ESP8266WebServer server(80);

// After
AsyncWebServer server(80);
```

### Handler Conversion

All handlers change signature from using the global `server` object to receiving an `AsyncWebServerRequest*` parameter.

**Pattern:**
```cpp
// Before
void handleApiState() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.send(200, F("application/json"), json);
}

// After
void handleApiState(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}
```

**Parameter access changes:**
- `server.hasArg(F("on"))` → `request->hasParam(F("on"), true)` (second arg `true` = POST body param)
- `server.arg(F("on"))` → `request->getParam(F("on"), true)->value()`
- `server.method() != HTTP_POST` checks are no longer needed — routes are registered with `HTTP_POST` method directly, so only matching methods reach the handler

### Route Registration

```cpp
// Before
server.on(F("/"), handleIndex);
server.on(F("/api/state"), HTTP_GET, handleApiState);
server.on(F("/api/auto"), HTTP_POST, handleApiAuto);

// After
server.on("/", HTTP_GET, handleIndex);
server.on("/api/state", HTTP_GET, handleApiState);
server.on("/api/auto", HTTP_POST, handleApiAuto);
```

Note: `AsyncWebServer::on()` does not accept `F()` macro for the path string. Use plain string literals.

### Remove handleNetwork()

Delete the `handleNetwork()` function (lines 164, 168–171 in .ino) and remove all calls to it:
- ~5 calls in `rgbw_stair_light.ino` (main loop, wait loops)
- ~17 calls in `parking.h` (inside animation loops)

With AsyncWebServer, requests are handled via interrupts — no manual pumping is needed.

`ArduinoOTA.handle()` still needs to be called in `loop()` — move it from `handleNetwork()` to the main loop directly.

### Remove server.handleClient()

No longer needed — delete from loop and from `handleApiReboot`.

### Refactor handleApiPlay (Flag-Based)

The current handler runs animations synchronously inside the request handler. AsyncWebServer handlers must return quickly (they execute in the TCP context).

**Solution:** Handler sets a flag, `loop()` picks it up:

```cpp
volatile int g_pendingPlayAnim = 0;  // 0 = none, 1–6 = animation to play

void handleApiPlay(AsyncWebServerRequest *request) {
  if (!request->hasParam("anim", true)) {
    request->send(400, F("text/plain"), F("anim=1..6"));
    return;
  }
  int anim = request->getParam("anim", true)->value().toInt();
  if (anim < 1 || anim > 6) {
    request->send(400, F("text/plain"), F("anim 1..6"));
    return;
  }
  g_pendingPlayAnim = anim;
  request->send(204);
}

// In loop(), after handleNetwork removal:
if (g_pendingPlayAnim > 0) {
  int anim = g_pendingPlayAnim;
  g_pendingPlayAnim = 0;
  // run animation (same logic as current handleApiPlay body)
}
```

### Refactor handleApiReboot (Flag-Based)

Like `handleApiPlay`, the reboot handler must not block. Use a flag that `loop()` checks:

```cpp
volatile bool g_pendingReboot = false;

void handleApiReboot(AsyncWebServerRequest *request) {
  g_pendingReboot = true;
  request->send(204);
}

// In loop():
if (g_pendingReboot) {
  delay(300);  // give async stack time to flush the 204 response
  ESP.restart();
}
```

### Thread Safety Note

AsyncWebServer on ESP8266 uses async TCP callbacks (from the lwIP stack), not a separate thread. The ESP8266 is single-core with cooperative multitasking. Shared variables (`g_pendingPlayAnim`, `g_pendingReboot`, `manual_r/g/b/w`, `automationOn`) are accessed from both async callbacks and `loop()`. On ESP8266 this is safe for simple flag/value reads and writes (single-core, no preemption between instructions), but avoid multi-step read-modify-write sequences in handlers.

### Phase 1 Verification

After Phase 1, all existing endpoints must work identically:
- `GET /` serves the same HTML
- `GET /api/state`, `/api/time`, `/api/log`, `/api/memory`, `/api/sysinfo` return same JSON formats
- `POST /api/auto`, `/api/color`, `/api/alloff`, `/api/play`, `/api/reboot` behave the same
- OTA upload works
- PIR-triggered animations work
- Night mode works
- Web UI functions identically (it's the same HTML)

---

## Phase 2: Web UI Redesign

### Layout: Collapsible Diagnostics

Top-to-bottom, single scrollable page:

1. **Header**: "Stair Light" title, date, time, uptime, last reboot
2. **Automation**: On/Off toggle buttons + night mode badge (conditional)
3. **RGBW Sliders**: One `<input type="range">` per channel (0–100%), colored LED indicator dot, percentage label
4. **All-Channel Presets**: Row of buttons: 0% / 25% / 50% / 75% / 100%
5. **Animation Buttons**: 6 individual color-coded buttons (one per animation)
6. **Reboot**: Single button
7. **Collapsible Diagnostics** (using `<details>/<summary>`):
   - Last 5 motions (table)
   - Memory status (table: heap/flash/RTC)
   - CPU / Runtime (table: freq, reset reason)
   - WiFi (table: SSID, RSSI, IP, gateway, DNS, channel, reconnects)
8. **Footer**: Firmware version

### RGBW Sliders

Each channel row:
```
[colored dot] =====[slider]===== [percentage%]
```

- HTML: `<input type="range" min="0" max="100" value="0">`
- Slider track styled to match channel color (red/green/blue/white)
- On `input` event, debounced to ~100ms, sends POST to `/api/color`

### API Addition: Direct Value Set

Add `v=<0-100>` parameter to `/api/color` for direct value setting (used by sliders):

```
POST /api/color
Body: c=r&v=70
```

Sets channel `r` to exactly 70%. Existing `a=minus`/`a=plus`/`a=toggle` parameters continue to work (backward compatible). The preset buttons use: `POST /api/color` with `all=<value>` (unchanged).

### Animation Buttons

Six buttons, each sending `POST /api/play` with `anim=1..6`:

| Button | Animation | Color Theme |
|--------|-----------|-------------|
| Random fade | `anim=1` | Green |
| Rainbow | `anim=2` | Blue/cyan |
| White ramp | `anim=3` | White/gray |
| Star sparkle | `anim=4` | Dark blue/purple |
| Birthday | `anim=5` | Pink/magenta |
| Night red | `anim=6` | Red |

Buttons show a pressed/active state on tap for visual feedback.

### Tiered Polling

**Fast tier — every 2 seconds:**
- `GET /api/state` — automation status, RGBW percentages, night mode flag
- `GET /api/time` — date, time, uptime, last reboot

**Slow tier — every 15 seconds, only when expanded:**
- `GET /api/log` — only polled when "Last 5 motions" `<details>` is open
- `GET /api/memory` — only polled when "Memory status" `<details>` is open
- `GET /api/sysinfo` — only polled when "CPU/Runtime" or "WiFi" `<details>` is open

When all diagnostic sections are collapsed, only 2 requests every 2 seconds (down from 5 every 1 second = 60% reduction in baseline traffic).

### Slider Debouncing

As the user drags a slider, `input` events fire rapidly. To avoid flooding the ESP:
- Track the latest value per channel
- Use a 100ms debounce timer — only send the POST when 100ms have passed since the last `input` event
- This means at most ~10 requests/second during active dragging, and the ESP always gets the final value

### Styling

- Dark theme preserved (`background: #1a1a1a`, light text)
- Same overall aesthetic as current UI (dark card backgrounds, colored accents)
- Mobile-first, max-width ~420px, responsive
- `<details>` elements styled to match the dark theme

---

## Files Modified

- `rgbw_stair_light/rgbw_stair_light.ino` — server library swap, handler conversion, HTML rewrite, new `v=` param in handleApiColor
- `rgbw_stair_light/parking.h` — remove all `handleNetwork()` calls

## Files Not Modified

- `rgbw_stair_light/credentials.h`
- `rgbw_stair_light/birthdays.h`
- Upload scripts (`upload-to-esp8266.sh`, `upload-to-esp8266-ota.sh`)
- Animation logic in `parking.h` (only `handleNetwork()` removal)
