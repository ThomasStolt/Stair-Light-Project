# External Control API + Configurable Settings — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an HTTP API for an external LAN process to drive the LED strip directly (red, red blinking, green dim-down), and make hostname + night-mode settings web-configurable and persisted to flash.

**Architecture:** Single Arduino sketch (`rgbw_stair_light/rgbw_stair_light.ino`) with helpers in `parking.h`. Async web handlers set `volatile` flags consumed by the blocking `loop()` (existing pattern). Settings live in a versioned struct persisted via ESP8266 EEPROM emulation; the struct fields ARE the runtime config (no separate globals — DRY). An external-control state machine runs non-blocking in `loop()` and suppresses motion triggers while active.

**Tech Stack:** ESP8266 Arduino core, ESPAsyncWebServer, Adafruit NeoPixel, `<EEPROM.h>` (core library), `arduino-cli`.

---

## Testing approach (read first)

This firmware has **no unit-test harness** and cannot be unit-tested off-device. Standard TDD does not apply. Each task therefore uses this loop instead:

1. Make the edit (exact code given).
2. **Compile-verify** — the project must compile clean:
   ```bash
   arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light
   ```
   (Run from the repo root. Equivalent to the VS Code task "Arduino: Compile (ESP8266 NodeMCU)". If `arduino-cli` is not on your PATH in a plain shell, run it from the VS Code task or your interactive shell.)
   Expected: `Sketch uses ... bytes` with no errors.
3. **Manual on-device verification** (where noted) — upload via `./upload-to-esp8266-ota.sh <host>` and check behaviour with `curl` / browser.
4. **Commit.**

Commit only the source/docs files named in each task — the working tree contains many unrelated `build_minimal/` artifacts; do **not** stage those.

---

## File structure

- **Modify** `rgbw_stair_light/rgbw_stair_light.ino` — all firmware changes (persistence, runtime config use, external-control state machine, new API handlers, route registration, web UI, version bump). This is the project's single sketch file; following the established all-in-one pattern.
- **Modify** `CHANGELOG.md` — new version entry.
- **Modify** `README.md` — document the external API and web-configurable settings.

No new files: the project deliberately keeps everything in the sketch + `parking.h`, and the new code is cohesive with existing handlers.

---

## Task 1: Settings persistence layer (EEPROM)

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (add `#include <EEPROM.h>`; add struct + globals + load/save after the `#include "parking.h"` line; call `loadSettings()` in `setup()`)

- [ ] **Step 1: Add the EEPROM include**

In `rgbw_stair_light.ino`, add the include next to the other includes (after line `#include <Ticker.h>`):

```cpp
#include <Ticker.h>
#include <EEPROM.h>
#include <time.h>
```

- [ ] **Step 2: Add the Settings struct, global, and load/save functions**

Immediately **after** the `#include "parking.h"` line (currently line 168), add:

```cpp
// ---- Persistent settings (EEPROM emulation) -------------------------------
// A small versioned struct. On first boot / blank flash / version change we
// fall back to the compiled-in defaults and re-save. The struct fields are the
// runtime config (read directly elsewhere) – no separate shadow globals.
#define SETTINGS_MAGIC   0x53544C31u  // 'STL1'
#define SETTINGS_VERSION 1
struct Settings {
  uint32_t magic;
  uint8_t  version;
  char     hostname[32];   // null-terminated, <=31 chars
  bool     nightEnabled;
  uint8_t  nightStart;     // hour 0–23 (inclusive)
  uint8_t  nightEnd;       // hour 0–23 (exclusive)
};
Settings g_settings;

void saveSettings() {
  EEPROM.put(0, g_settings);
  EEPROM.commit();
}

void loadSettings() {
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, g_settings);
  if (g_settings.magic != SETTINGS_MAGIC || g_settings.version != SETTINGS_VERSION) {
    // First boot / blank / version mismatch → compiled defaults
    g_settings.magic   = SETTINGS_MAGIC;
    g_settings.version = SETTINGS_VERSION;
    strncpy(g_settings.hostname, OTA_HOSTNAME, sizeof(g_settings.hostname) - 1);
    g_settings.hostname[sizeof(g_settings.hostname) - 1] = '\0';
    g_settings.nightEnabled = true;
    g_settings.nightStart   = NIGHT_HOUR_START;
    g_settings.nightEnd     = NIGHT_HOUR_END;
    saveSettings();
  }
}
```

(`saveSettings` is defined before `loadSettings`; in an `.ino` file function order does not matter because the Arduino preprocessor auto-prototypes, but this order also satisfies a plain C reading.)

- [ ] **Step 3: Call `loadSettings()` early in `setup()`**

In `setup()`, right after the reset-info caching lines:

```cpp
  // Cache reset reason/info before WiFi stack overwrites them
  s_resetReason = ESP.getResetReason();
  s_resetInfo   = ESP.getResetInfo();

  // Load persisted settings (hostname, night mode) – falls back to defaults
  loadSettings();
```

- [ ] **Step 4: Compile-verify**

Run: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`
Expected: compiles clean, no errors. (Globals are unused so far — that is fine; Task 2 wires them in.)

- [ ] **Step 5: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add EEPROM-backed Settings struct (hostname, night mode)"
```

---

## Task 2: Use persisted settings for night mode + hostname

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (`isNightMode()`, and `setup()` hostname usage)

- [ ] **Step 1: Make `isNightMode()` read the settings**

Replace the body of `isNightMode()` (currently around line 611) with:

```cpp
bool isNightMode(long unixTimeUtc) {
  if (!g_settings.nightEnabled) return false;
  if (unixTimeUtc <= 0) return false;
  time_t t = (time_t)(unixTimeUtc + TIMEZONE_OFFSET_SEC(unixTimeUtc));
  struct tm *tm = gmtime(&t);
  if (!tm) return false;
  int hour = tm->tm_hour;
  return (hour >= g_settings.nightStart && hour < g_settings.nightEnd);
}
```

- [ ] **Step 2: Set the WiFi (DHCP) hostname before connecting**

In `setup()`, immediately **before** `WiFi.begin(WIFI_SSID, WIFI_PASS);`:

```cpp
  // Register our name on the network (DHCP) using the persisted hostname
  WiFi.hostname(g_settings.hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
```

- [ ] **Step 3: Use the persisted hostname for OTA**

Replace `ArduinoOTA.setHostname(OTA_HOSTNAME);` (around line 701) with:

```cpp
    ArduinoOTA.setHostname(g_settings.hostname);
```

- [ ] **Step 4: Compile-verify**

Run: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`
Expected: compiles clean.

- [ ] **Step 5: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Drive night mode and hostname from persisted settings"
```

---

## Task 3: External-control state machine in `loop()`

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (state globals near the other volatile flags; `applyExtCommand`/`serviceExtControl` helpers; integration in `loop()`)

- [ ] **Step 1: Add the state-machine globals**

After the existing pending-flag globals (after the `volatile bool g_pendingReboot = false;` line, ~line 104), add:

```cpp
// External control (parking/garage signalling). Web handler sets g_pendingExtCmd;
// loop() applies it and drives the strip non-blocking while g_extActive is true.
enum ExtMode { EXT_NONE = 0, EXT_RED, EXT_RED_BLINK, EXT_GREEN_FADE };
volatile int  g_pendingExtCmd = 0;   // 0=none,1=red,2=red_blink,3=green_fade,4=clear
ExtMode       g_extMode    = EXT_NONE;
bool          g_extActive  = false;
unsigned long g_extStartMs = 0;      // green_fade start time
unsigned long g_extBlinkMs = 0;      // red_blink last toggle time
bool          g_extBlinkOn = false;
```

- [ ] **Step 2: Add the apply + service helpers**

Add these two functions just **before** `void setup() {` (after `updateNightRed()`, ~line 631). They reuse the existing `manualPctToGamma()` and `setAll()` helpers:

```cpp
// Apply a newly received external command (sets mode + initial strip state).
void applyExtCommand(int cmd) {
  unsigned long now = millis();
  switch (cmd) {
    case 1: // red – solid, hold
      g_extMode = EXT_RED;  g_extActive = true;
      setAll(255, 0, 0, 0); strip.show();
      break;
    case 2: // red_blink – 500 ms toggle
      g_extMode = EXT_RED_BLINK; g_extActive = true;
      g_extBlinkMs = now; g_extBlinkOn = true;
      setAll(255, 0, 0, 0); strip.show();
      break;
    case 3: // green_fade – full→off over ~30 s, then auto-clear
      g_extMode = EXT_GREEN_FADE; g_extActive = true;
      g_extStartMs = now;
      setAll(0, manualPctToGamma(100), 0, 0); strip.show();
      break;
    case 4: // clear – release override, LEDs off
    default:
      g_extMode = EXT_NONE; g_extActive = false;
      setAll(0, 0, 0, 0); strip.show();
      break;
  }
}

// Per-iteration servicing of an active external command (non-blocking).
void serviceExtControl() {
  unsigned long now = millis();
  if (g_extMode == EXT_RED_BLINK) {
    if (now - g_extBlinkMs >= 500uL) {
      g_extBlinkMs = now;
      g_extBlinkOn = !g_extBlinkOn;
      setAll(g_extBlinkOn ? 255 : 0, 0, 0, 0);
      strip.show();
    }
  } else if (g_extMode == EXT_GREEN_FADE) {
    unsigned long elapsed = now - g_extStartMs;
    if (elapsed >= 30000uL) {
      setAll(0, 0, 0, 0); strip.show();
      g_extMode = EXT_NONE;
      g_extActive = false;            // normal behaviour resumes next iteration
    } else {
      int pct = (int)((30000uL - elapsed) * 100uL / 30000uL);
      setAll(0, manualPctToGamma((uint8_t)pct), 0, 0);
      strip.show();
    }
  }
  // EXT_RED: nothing to do per-frame (already solid)
}
```

- [ ] **Step 3: Integrate into `loop()`**

In `loop()`, **after** the pending-animation block (after its closing `}` at ~line 809) and **before** `bool night = isNightMode(currenttime);`, insert:

```cpp
    // External control overrides motion detection while active
    if (g_pendingExtCmd > 0) {
      applyExtCommand(g_pendingExtCmd);
      g_pendingExtCmd = 0;
    }
    if (g_extActive) {
      serviceExtControl();
      yield();
      delay(50);
      continue;   // skip PIR / night / day handling this iteration
    }
```

(`ArduinoOTA.handle()` and the pending-reboot check run earlier in the loop body, so OTA and reboot still work while an external command is active.)

- [ ] **Step 4: Compile-verify**

Run: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`
Expected: compiles clean. (Nothing sets `g_pendingExtCmd` yet — wired in Task 4.)

- [ ] **Step 5: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add external-control state machine (red / red_blink / green_fade)"
```

---

## Task 4: `/api/ext` endpoint

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (handler + route)

- [ ] **Step 1: Add the handler**

Add after `handleApiPlay()` (~line 441):

```cpp
// POST /api/ext – external process drives the strip directly.
// state = red | red_blink | green_fade | clear  (suppresses motion while active)
void handleApiExt(AsyncWebServerRequest *request) {
  if (!request->hasParam(F("state"), true)) {
    request->send(400, F("text/plain"), F("state=red|red_blink|green_fade|clear"));
    return;
  }
  String s = request->getParam(F("state"), true)->value();
  int cmd = 0;
  if      (s == F("red"))        cmd = 1;
  else if (s == F("red_blink"))  cmd = 2;
  else if (s == F("green_fade")) cmd = 3;
  else if (s == F("clear"))      cmd = 4;
  else { request->send(400, F("text/plain"), F("state=red|red_blink|green_fade|clear")); return; }
  g_pendingExtCmd = cmd;
  request->send(204);
}
```

- [ ] **Step 2: Register the route**

In `setup()`, next to the other `server.on(...)` calls (after the `/api/play` line ~733):

```cpp
  server.on("/api/ext", HTTP_POST, handleApiExt);
```

- [ ] **Step 3: Compile-verify**

Run: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`
Expected: compiles clean.

- [ ] **Step 4: Manual on-device verification**

Upload (`./upload-to-esp8266-ota.sh <host-or-ip>`), then with the device IP/host:

```bash
curl -X POST -d state=red        http://<host>/api/ext   # solid red, motion ignored
curl -X POST -d state=red_blink  http://<host>/api/ext   # blinks every 500 ms
curl -X POST -d state=green_fade http://<host>/api/ext   # green dims to off over ~30 s, then normal resumes
curl -X POST -d state=clear      http://<host>/api/ext   # off + normal resumes immediately
curl -X POST -d state=bogus      http://<host>/api/ext   # -> 400
```
Confirm: PIR motion does nothing while red/red_blink is held; after `green_fade` completes (or `clear`), normal stair automation (or night mode, if within night hours) works again.

- [ ] **Step 5: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add POST /api/ext external control endpoint"
```

---

## Task 5: `/api/settings` GET + POST endpoints

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (two handlers + two routes)

- [ ] **Step 1: Add the GET and POST handlers**

Add after `handleApiSysinfo()` (~line 609):

```cpp
// GET /api/settings – current configurable settings for the web UI
void handleApiSettingsGet(AsyncWebServerRequest *request) {
  String hn = g_settings.hostname;
  hn.replace("\"", "'");   // keep JSON valid
  String json = "{\"hostname\":\"";
  json += hn;
  json += "\",\"night_enabled\":";
  json += g_settings.nightEnabled ? "1" : "0";
  json += ",\"night_start\":";
  json += g_settings.nightStart;
  json += ",\"night_end\":";
  json += g_settings.nightEnd;
  json += "}";
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}

// POST /api/settings – update any of hostname / night_enabled / night_start / night_end
void handleApiSettingsPost(AsyncWebServerRequest *request) {
  bool changed = false;
  if (request->hasParam(F("hostname"), true)) {
    String hn = request->getParam(F("hostname"), true)->value();
    hn.trim();
    if (hn.length() == 0 || hn.length() > 31) {
      request->send(400, F("text/plain"), F("hostname 1..31 chars")); return;
    }
    strncpy(g_settings.hostname, hn.c_str(), sizeof(g_settings.hostname) - 1);
    g_settings.hostname[sizeof(g_settings.hostname) - 1] = '\0';
    changed = true;
  }
  if (request->hasParam(F("night_enabled"), true)) {
    g_settings.nightEnabled = (request->getParam(F("night_enabled"), true)->value().toInt() != 0);
    changed = true;
  }
  if (request->hasParam(F("night_start"), true)) {
    int v = request->getParam(F("night_start"), true)->value().toInt();
    if (v < 0 || v > 23) { request->send(400, F("text/plain"), F("night_start 0..23")); return; }
    g_settings.nightStart = (uint8_t)v;
    changed = true;
  }
  if (request->hasParam(F("night_end"), true)) {
    int v = request->getParam(F("night_end"), true)->value().toInt();
    if (v < 0 || v > 23) { request->send(400, F("text/plain"), F("night_end 0..23")); return; }
    g_settings.nightEnd = (uint8_t)v;
    changed = true;
  }
  if (changed) saveSettings();
  request->send(204);
}
```

- [ ] **Step 2: Register the routes**

In `setup()`, after the `/api/sysinfo` route (~line 738):

```cpp
  server.on("/api/settings", HTTP_GET,  handleApiSettingsGet);
  server.on("/api/settings", HTTP_POST, handleApiSettingsPost);
```

- [ ] **Step 3: Compile-verify**

Run: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`
Expected: compiles clean.

- [ ] **Step 4: Manual on-device verification**

```bash
curl http://<host>/api/settings
# {"hostname":"stairlight-testbed","night_enabled":1,"night_start":1,"night_end":6}
curl -X POST -d 'night_start=2&night_end=7&night_enabled=1' http://<host>/api/settings   # -> 204
curl -X POST -d 'night_start=99' http://<host>/api/settings   # -> 400
curl -X POST -d 'hostname=stairlight' http://<host>/api/settings   # -> 204
curl http://<host>/api/settings   # reflects new values
```
Reboot the device (`curl -X POST http://<host>/api/reboot`) and confirm `GET /api/settings` still returns the saved values (persistence works).

- [ ] **Step 5: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add GET/POST /api/settings endpoints with persistence"
```

---

## Task 6: Web UI — Settings section + dynamic night badge

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (`handleIndex()` PROGMEM HTML + JS)

- [ ] **Step 1: Make the night badge window dynamic**

In the embedded HTML (in `handleIndex`), replace the night badge line (currently uses `NIGHT_HOUR_START_STR` / `NIGHT_HOUR_END_STR`):

```cpp
    "<p id=nightBadge style=\"display:none;margin:0.4rem 0 0;padding:0.4rem 0.8rem;border-radius:8px;background:rgba(180,40,40,0.25);border:1px solid #a33;font-size:0.9rem;color:#f88;\">Night mode active <span id=nightWindow></span></p>"
```

- [ ] **Step 2: Add the Settings `<details>` section**

Insert this block **before** the firmware footer line (`"<p style=margin-top:1rem;font-size:0.75rem;color:#666;>Firmware v" FW_VERSION ...`):

```cpp
    "<details id=detailSettings><summary>Settings</summary><div class=inner>"
    "<div style=margin-bottom:0.6rem;>"
    "<label style=display:block;margin-bottom:0.2rem;font-size:0.9rem;>Hostname</label>"
    "<input type=text id=setHostname style=\"width:100%;box-sizing:border-box;padding:0.4rem;background:#111;color:#eee;border:1px solid #444;border-radius:6px;\">"
    "<div style=font-size:0.75rem;color:#888;margin-top:0.2rem;>Applies after reboot</div>"
    "</div>"
    "<div style=margin-bottom:0.6rem;><label style=font-size:0.9rem;><input type=checkbox id=setNightEnabled> Night mode enabled</label></div>"
    "<div class=row style=justify-content:flex-start;>"
    "<label style=font-size:0.9rem;>Start <input type=number id=setNightStart min=0 max=23 style=\"width:3.5em;background:#111;color:#eee;border:1px solid #444;border-radius:6px;padding:0.3rem;\">:00</label>"
    "<label style=font-size:0.9rem;>End <input type=number id=setNightEnd min=0 max=23 style=\"width:3.5em;background:#111;color:#eee;border:1px solid #444;border-radius:6px;padding:0.3rem;\">:00</label>"
    "</div>"
    "<div class=row style=justify-content:flex-start;margin-top:0.5rem;>"
    "<button type=button class=\"btn preset\" id=setSave>Save settings</button>"
    "<span id=setStatus style=font-size:0.85rem;></span></div>"
    "</div></details>"
```

- [ ] **Step 3: Add the settings load/save JavaScript**

In the `<script>` block, add this function near the other `function ...` definitions (e.g. after `loadSlow(){...}`):

```cpp
    "function loadSettings(){"
    "fetch('/api/settings').then(function(r){return r.json();}).then(function(s){"
    "document.getElementById('setHostname').value=s.hostname||'';"
    "document.getElementById('setNightEnabled').checked=!!s.night_enabled;"
    "document.getElementById('setNightStart').value=s.night_start;"
    "document.getElementById('setNightEnd').value=s.night_end;"
    "document.getElementById('nightWindow').textContent='('+s.night_start+':00 - '+s.night_end+':00)';"
    "});}"
    "document.getElementById('setSave').onclick=function(){"
    "var body='hostname='+encodeURIComponent(document.getElementById('setHostname').value)"
    "+'&night_enabled='+(document.getElementById('setNightEnabled').checked?1:0)"
    "+'&night_start='+document.getElementById('setNightStart').value"
    "+'&night_end='+document.getElementById('setNightEnd').value;"
    "var st=document.getElementById('setStatus');st.textContent='Saving...';st.style.color='#fc8';"
    "post('/api/settings',body).then(function(r){"
    "if(r.status===204){st.textContent='Saved';st.style.color='#9f9';loadSettings();}"
    "else{st.textContent='Invalid';st.style.color='#f88';}"
    "});};"
```

- [ ] **Step 4: Call `loadSettings()` on page load**

Find the init lines near the end of the script (`loadFast();loadSlow();`) and add the settings load:

```cpp
    "loadFast();loadSlow();loadSettings();"
```

- [ ] **Step 5: Compile-verify**

Run: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`
Expected: compiles clean.

- [ ] **Step 6: Manual on-device verification**

Upload, open `http://<host>/` in a browser:
- "Settings" section shows current hostname + night fields populated from the device.
- Change night start/end + toggle, click "Save settings" → "Saved"; verify `GET /api/settings` reflects it and night badge window text matches.
- The night badge (top) shows the configured window when night mode is active.
- Change hostname, save, reboot → hostname persists.

- [ ] **Step 7: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add web UI Settings section (hostname, night mode) + dynamic badge"
```

---

## Task 7: Version bump + documentation

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (`FW_VERSION`)
- Modify: `CHANGELOG.md`
- Modify: `README.md`

- [ ] **Step 1: Bump the firmware version**

Change `#define FW_VERSION "2.0.0"` (line ~158) to:

```cpp
#define FW_VERSION "2.1.0"
```

- [ ] **Step 2: Add a CHANGELOG entry**

Insert at the top of `CHANGELOG.md`, directly under the `# Changelog` heading:

```markdown
## 2.1.0 – 2026-06-06

- **External control API** – `POST /api/ext` with `state=red|red_blink|green_fade|clear` lets another LAN process drive the strip directly (solid red, 500 ms red blink, ~30 s green dim-down). Motion detection is suppressed while active; normal behaviour (or night mode) resumes after `green_fade`/`clear`.
- **Web-configurable settings** – Hostname and night mode (enable toggle + start/end hours) are now editable in the web UI under "Settings".
- **Persistent settings** – Hostname and night-mode settings are stored in EEPROM and survive reboot (fall back to compiled defaults on first boot). Hostname changes apply after reboot.
- **New endpoints** – `GET`/`POST /api/settings`.
```

- [ ] **Step 3: Document the API and settings in README.md**

Under `### 1. WiFi and hostname`, after the line `Without \`credentials.h\` the build fails.` add:

```markdown

> The hostname can also be changed at runtime in the web UI (**Settings** section); it is stored in flash and applied on the next reboot. `OTA_HOSTNAME` in `credentials.h` is only the default used on first boot.
```

Then add a new section immediately **after** the `## Night mode (1–6 h)` section (before `## Sketch configuration`):

```markdown
## External control API

Another process on the same network can drive the strip directly, bypassing motion
detection — e.g. a parking/garage helper signalling stop/go.

`POST /api/ext` with form field `state`:

| `state`      | Effect                                                        |
|--------------|--------------------------------------------------------------|
| `red`        | Solid red, held until the next command                        |
| `red_blink`  | Red blinking every 500 ms                                     |
| `green_fade` | Green dimming from full to off over ~30 s, then auto-clears   |
| `clear`      | LEDs off immediately, override released                       |

While a command is active, motion detection is suppressed. After `green_fade` finishes
(or `clear`), normal behaviour resumes — daytime automation, or night mode if within
the configured night hours. No authentication (trusted LAN only).

```bash
curl -X POST -d state=red        http://<host>/api/ext
curl -X POST -d state=red_blink  http://<host>/api/ext
curl -X POST -d state=green_fade http://<host>/api/ext
curl -X POST -d state=clear      http://<host>/api/ext
```

Night mode (enable + start/end hours) and the hostname are configurable in the web UI
**Settings** section and persist across reboots.
```

- [ ] **Step 4: Compile-verify**

Run: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`
Expected: compiles clean (footer now shows v2.1.0).

- [ ] **Step 5: Commit**

```bash
git add rgbw_stair_light/rgbw_stair_light.ino CHANGELOG.md README.md
git commit -m "Bump to 2.1.0; document external API and configurable settings"
```

---

## Self-review checklist (completed by plan author)

- **Spec coverage:** external API independent commands (Task 4) ✓; red/red_blink/green_fade(30s)/clear behaviour (Task 3) ✓; motion suppressed while active, resume incl. night mode after fade (Task 3 `continue` + auto-clear) ✓; persistence via EEPROM (Task 1) ✓; hostname web-config applied on reboot + `WiFi.hostname`/OTA (Task 2, 6) ✓; night enable + start/end hours config (Task 5, 6) ✓; no auth ✓; whole-hour granularity ✓.
- **Placeholder scan:** none — every code/step is concrete.
- **Type/name consistency:** `g_settings`, `g_pendingExtCmd`, `g_extMode`/`g_extActive`/`g_extStartMs`/`g_extBlinkMs`/`g_extBlinkOn`, `applyExtCommand`/`serviceExtControl`, `handleApiExt`, `handleApiSettingsGet`/`handleApiSettingsPost`, `loadSettings`/`saveSettings` used consistently across tasks; `manualPctToGamma`/`setAll`/`strip.show` match existing code.
```

