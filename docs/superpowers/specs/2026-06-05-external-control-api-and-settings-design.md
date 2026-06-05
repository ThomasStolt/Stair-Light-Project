# External Control API + Configurable Settings — Design

**Date:** 2026-06-05
**Component:** `rgbw_stair_light/rgbw_stair_light.ino` (+ `parking.h`, `credentials.h`)
**Status:** Approved

## Goal

Two related additions to the Stair Light firmware:

1. **External control API** — a simple HTTP endpoint another process on the LAN can
   call to drive the strip directly (red, red blinking, green dim-down), bypassing
   motion detection. Intended for a parking/garage-style signalling scenario where an
   external controller decides moment-to-moment what to show.
2. **Web-configurable settings** — expose hostname and night-mode parameters (enable
   toggle + start/end hours) in the web UI, persisted to flash so they survive reboot.

## Decisions (from brainstorming)

- **API shape:** independent commands, not one scripted sequence. The external process
  decides timing and sends each state when it wants.
- **Persistence:** persist hostname + night settings to flash (EEPROM emulation).
- **After green fade:** clear the override; normal behaviour resumes automatically —
  day automation if on, or night mode if currently within night hours.
- **Auth:** none. Trusted LAN only, matching the existing API.
- **Night controls:** enable toggle **and** editable start/end hours. When disabled,
  night mode never engages regardless of time.
- **Night time granularity:** whole hours (0–23), matching the existing `isNightMode`.

## Architecture

The firmware is a single Arduino sketch with helpers in `parking.h`. Async web handlers
set `volatile` flags consumed by the blocking `loop()` (existing pattern for animations
and reboot). This design follows the same pattern so timed/blocking effects run in
`loop()` and the web server stays responsive.

### Component 1 — Settings persistence (EEPROM)

A small versioned struct stored in the ESP8266 EEPROM (flash sector emulation):

```c
struct Settings {
  uint32_t magic;        // sentinel; detects blank / first boot
  uint8_t  version;      // struct version for future migrations
  char     hostname[32]; // null-terminated
  bool     nightEnabled;
  uint8_t  nightStart;   // hour 0–23 (inclusive)
  uint8_t  nightEnd;     // hour 0–23 (exclusive)
};
```

- `loadSettings()` — called early in `setup()`. `EEPROM.begin(sizeof(Settings))` then
  `EEPROM.get`. If `magic` does not match the expected sentinel (first boot or blank
  flash), populate the struct from the compiled-in defaults (`OTA_HOSTNAME`,
  `NIGHT_HOUR_START`, `NIGHT_HOUR_END`, `nightEnabled = true`) and call `saveSettings()`.
- `saveSettings()` — `EEPROM.put` + `EEPROM.commit()`.
- The existing `#define`s (`OTA_HOSTNAME`, `NIGHT_HOUR_START`, `NIGHT_HOUR_END`) remain
  as the **default** source only.

**Interface:** `void loadSettings()`, `void saveSettings()`. Depends on `<EEPROM.h>`.

### Component 2 — Runtime config variables

New globals, initialised by `loadSettings()`:

- `String g_hostname` (or `char[32]`)
- `bool g_nightEnabled`
- `uint8_t g_nightStart`
- `uint8_t g_nightEnd`

Changes:
- `isNightMode()` reads `g_nightEnabled`, `g_nightStart`, `g_nightEnd` instead of the
  `#define`s. Returns `false` immediately if `!g_nightEnabled`.
- In `setup()`: `WiFi.hostname(g_hostname.c_str())` before `WiFi.begin(...)` so the
  device registers its name via DHCP, and `ArduinoOTA.setHostname(g_hostname.c_str())`.
- Night-time / enable changes apply **immediately** (next `loop()` iteration reads the
  globals). Hostname changes take effect **on next reboot** (used at startup by
  WiFi/OTA/mDNS) — the UI states this.

### Component 3 — External-control state machine

State (file-scope):

```c
enum ExtMode { EXT_NONE, EXT_RED, EXT_RED_BLINK, EXT_GREEN_FADE };
volatile int  g_pendingExtCmd = 0; // 0=none, else maps to a command incl. "clear"
ExtMode       g_extMode = EXT_NONE;
bool          g_extActive = false;
unsigned long g_extStartMs = 0;    // for green_fade timing
unsigned long g_extBlinkMs = 0;    // for red_blink toggle timing
bool          g_extBlinkOn = false;
```

Behaviour, serviced each `loop()` iteration (non-blocking, `millis()`-based):

| state        | behaviour |
|--------------|-----------|
| `red`        | solid red at full brightness, held until next command; `g_extActive=true` |
| `red_blink`  | red toggles on/off every 500 ms; `g_extActive=true` |
| `green_fade` | green ramps full→off over ~30 000 ms, then LEDs off, `g_extActive=false` (normal resumes) |
| `clear`      | LEDs off, `g_extActive=false`, `g_extMode=EXT_NONE` (normal resumes) |

- While `g_extActive` is true, `loop()` **skips both day and night motion triggers** and
  the night-idle clearing logic, so the external signal is never overridden by a PIR
  event. `ArduinoOTA.handle()` and the web server continue to run.
- Colour values use full red / green via the existing gamma table for consistency with
  other effects. The green fade computes
  `pct = 100 * (30000 - elapsed) / 30000` and applies the gamma-mapped green.
- A new `red`/`red_blink`/`green_fade` command issued while one is already active simply
  replaces the current mode.

### Component 4 — API endpoint `/api/ext`

`POST /api/ext` with form/query param `state`:
- Accepts `red`, `red_blink`, `green_fade`, `clear`.
- Validates `state`; unknown value → `400 text/plain`.
- Maps to `g_pendingExtCmd` and returns `204` (consistent with `/api/play`,
  `/api/reboot`). The actual effect runs in `loop()`.
- No authentication (trusted LAN).

### Component 5 — Web UI: Settings section + endpoints

New collapsible `<details>` "Settings" section (matching the existing diagnostics
styling) added to the embedded `PROGMEM` page:

- **Hostname:** text input + Save button, with an "applies after reboot" hint.
- **Night mode:** enable checkbox, start-hour input, end-hour input + Save button.
- The existing night badge text becomes dynamic, showing the configured hours.

New endpoints:
- `GET /api/settings` → `{"hostname":"...","night_enabled":0|1,"night_start":H,"night_end":H}`
  (`Cache-Control: no-store`).
- `POST /api/settings` → accepts any of `hostname`, `night_enabled`, `night_start`,
  `night_end`; validates (hostname length ≤ 31 and non-empty; hours 0–23); updates the
  runtime globals; calls `saveSettings()`; returns `204` (or `400` on invalid input).

Route registration added in `setup()` next to the existing `server.on(...)` calls.

## Data flow

```
External process ──POST /api/ext state=red───────────► handler sets g_pendingExtCmd ─► loop() drives strip
Web UI ──POST /api/settings──► handler validates ─► updates g_* globals ─► saveSettings() ─► EEPROM
Boot ──► loadSettings() ─► g_* globals ─► WiFi.hostname / ArduinoOTA.setHostname / isNightMode()
```

## Error handling

- `loadSettings()` falls back to compiled defaults on magic/version mismatch and
  re-saves, so corrupt/blank flash self-heals.
- `/api/ext` and `/api/settings` reject invalid params with `400` and a short message.
- Hostname validation prevents empty or over-length values from being stored.
- External override never blocks OTA or the web server (effects are non-blocking in
  `loop()`).

## Testing

No unit-test harness exists for this firmware. Verification is:

1. **Compile clean** (arduino-cli / PlatformIO), no warnings introduced.
2. **External API (manual, `curl`):** each `state=red|red_blink|green_fade|clear`
   produces the expected LED behaviour; confirm the ~30 s green dim-down and that
   motion is ignored while active; confirm normal behaviour (incl. night mode) resumes
   after fade/clear.
3. **Settings (manual):** change night enable/start/end in the UI → takes effect
   immediately; change hostname → save, reboot, confirm new hostname on the router and
   for OTA; confirm all values **persist across reboot**.

## Out of scope (YAGNI)

- Authentication / tokens on the API.
- Minute-level night-time precision (whole hours only).
- LittleFS/JSON config (EEPROM struct is sufficient for these few values).
- A scripted single-call sequence (commands are independent).
