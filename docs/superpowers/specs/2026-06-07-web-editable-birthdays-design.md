# Web-Editable Birthdays — Design

**Date:** 2026-06-07
**Component:** `rgbw_stair_light/rgbw_stair_light.ino` (+ `birthdays.h`, README, CHANGELOG)
**Status:** Approved

## Goal

Let the user view and edit the birthday list from the Web UI. Birthdays are currently
compile-time constants in the gitignored `birthdays.h`; this makes them runtime-editable
and persisted to flash, with a per-entry name shown in the UI.

## Decisions (from brainstorming)

- **Edit model:** edit the whole list in the UI, then one **Save** writes it atomically
  (replace-all). Add appends a blank row; × removes a row.
- **Name field:** each birthday stores an optional short name, shown in the UI to identify
  the entry. The name is **UI-only** — the birthday animation stays generic and does not
  use it.
- **Max count:** 20 birthdays.
- **Storage:** **separate EEPROM region** for birthdays (Approach A), independent of the
  existing `Settings` region. This preserves the hostname/night settings already saved on
  the device (no reset on upgrade).
- `birthdays.h` becomes the **default source only**, consistent with how the hostname and
  night-mode defaults work.

## Architecture

Single Arduino sketch. The settings persistence layer added in v2.1.0 stores a `Settings`
struct at EEPROM offset 0 (magic `STL1`, version 1). This feature adds a second,
independent EEPROM region for birthdays at a fixed offset, plus a GET/POST API and a Web
UI section. Async web handlers validate input and persist; `loop()`/`isBirthdayToday()`
read the runtime copy.

### Component 1 — EEPROM layout (small refactor)

- Define `EEPROM_SIZE` (1024 bytes) and use it in **every** `EEPROM.begin()` call,
  replacing the current `EEPROM.begin(sizeof(Settings))`. On the ESP8266 core, `begin()`
  re-reads the whole sector into the RAM buffer, so any handler that writes only its own
  region leaves the other region intact (read-from-flash, write-back-unchanged).
- Layout:
  - `EEPROM_SETTINGS_ADDR = 0` (existing `Settings`, unchanged: same struct, magic, and
    version — so existing saved settings survive this upgrade).
  - `EEPROM_BIRTHDAYS_ADDR = 128` (fixed offset; leaves headroom for `Settings` growth).
- New struct + global:
```c
#define BIRTHDAYS_MAX     20
#define BIRTHDAY_NAME_LEN 20          // includes null terminator (<=19 visible chars)
#define BIRTHDAYS_MAGIC   0x53544231u // 'STB1'
#define BIRTHDAYS_VERSION 1
struct BirthdayStore {
  uint32_t magic;
  uint8_t  version;
  uint8_t  count;                     // 0..BIRTHDAYS_MAX
  struct {
    uint8_t month;                    // 1..12
    uint8_t day;                      // 1..31
    char    name[BIRTHDAY_NAME_LEN];  // null-terminated, may be empty
  } items[BIRTHDAYS_MAX];
};
BirthdayStore g_birthdays;
```
- `loadBirthdays()`: `EEPROM.begin(EEPROM_SIZE)`, `EEPROM.get(EEPROM_BIRTHDAYS_ADDR, g_birthdays)`.
  If `magic`/`version` mismatch (first boot / blank / schema change), seed from the
  compile-time defaults in `birthdays.h` (`BIRTHDAY_COUNT`, `BIRTHDAYS[][2]`), with names
  set to empty strings, clamp count to `BIRTHDAYS_MAX`, set magic/version, and
  `saveBirthdays()`.
- `saveBirthdays()`: `EEPROM.begin(EEPROM_SIZE)`, `EEPROM.put(EEPROM_BIRTHDAYS_ADDR, g_birthdays)`,
  `EEPROM.commit()`.
- Estimated size: 6 + 20×22 = 446 bytes, well within `EEPROM_SIZE` and the 4096-byte sector.

**Interface:** `void loadBirthdays()`, `void saveBirthdays()`. Depends on `<EEPROM.h>`,
`birthdays.h`.

### Component 2 — Behaviour wiring

- `isBirthdayToday(long unixTimeUtc)` iterates `g_birthdays.items[0..g_birthdays.count]`
  comparing `month`/`day` against the current local date (same date math as today),
  instead of the static `BIRTHDAYS[]`.
- `loadBirthdays()` is called in `setup()` next to `loadSettings()`.
- The birthday animation (`birthday()` in `parking.h`) is unchanged and does not use names.
- `birthdays.h` is retained solely as the default seed for `loadBirthdays()`.

### Component 3 — API

- `GET /api/birthdays` → JSON array `[{"m":<1-12>,"d":<1-31>,"name":"<sanitised>"},...]`,
  `Cache-Control: no-store`. Names are sanitised for JSON (`"` → `'`; backslashes/control
  chars cannot occur because POST validation restricts the charset — see below).
- `POST /api/birthdays` — **replace-all**, `application/x-www-form-urlencoded` body with
  indexed params: `count=N`, then for `i` in `0..N-1`: `m<i>`, `d<i>`, `n<i>`
  (name URL-encoded by the browser). Validation:
  - `count` present, integer `0..BIRTHDAYS_MAX`; else `400`.
  - each `m<i>` `1..12`, each `d<i>` `1..31`; else `400`.
  - each `n<i>` length `0..19`; characters restricted to letters, digits, space, and
    `-` `_` `.` (keeps JSON output safe and avoids control chars); else `400`.
  - On any invalid field: `400 text/plain` with a short message, **no partial write**
    (build into a local temp, only commit to `g_birthdays` after all entries validate).
  - On success: update `g_birthdays`, `saveBirthdays()`, `204`.
- Routes registered in `setup()` next to the other `server.on(...)` calls.

### Component 4 — Web UI

- New collapsible `<details id=detailBirthdays>` "Birthdays" section (matching existing
  styling), containing a `<div id=bdayRows>` (one row per entry: month number input,
  day number input, name text input, and a × remove button), an **Add** button that
  appends a blank row (capped at `BIRTHDAYS_MAX`), a **Save** button, and a status span.
- JS:
  - `loadBirthdays()` fetches `/api/birthdays` and renders rows.
  - A `renderBdayRow(m,d,name)` helper creates a row with its × handler.
  - Save serialises all rows into the indexed body and POSTs; shows
    "Saving…/Saved/Invalid"; reloads on `204`.
  - Called once at page load (like `loadSettings()`).

### Component 5 — Version + docs

- Bump `FW_VERSION` to `2.2.0`.
- CHANGELOG `2.2.0` entry; README: document `GET/POST /api/birthdays` and the Birthdays UI;
  note birthdays are runtime-editable and `birthdays.h` is the first-boot default.

## Data flow

```
Boot → loadBirthdays() → g_birthdays (or defaults from birthdays.h) → isBirthdayToday()
Web UI Birthdays section → GET /api/birthdays → render rows
Save → POST /api/birthdays (count + m/d/n indexed) → validate → g_birthdays → saveBirthdays() → EEPROM region @128
```

## Error handling

- `loadBirthdays()` self-heals on magic/version mismatch or blank flash (re-seeds defaults
  and re-saves).
- POST rejects invalid input with `400` and writes nothing on failure (validate-then-commit).
- Name charset restriction guarantees valid JSON on GET.
- Separate EEPROM regions + full-sector re-read on each `begin()` prevent the settings and
  birthday regions from corrupting each other.

## Testing

No unit-test harness (firmware). Verification:
1. **Compile clean** (`arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`).
2. **Manual on-device:** open Birthdays section → shows the 3 existing dates; add a name;
   add a row; remove a row; Save → `GET /api/birthdays` reflects it; reboot → persisted.
   Confirm a configured date still triggers the birthday animation. Confirm hostname/night
   settings saved previously are still intact after this upgrade.

## Out of scope (YAGNI)

- Using the name in the birthday animation or motion log.
- Per-entry immediate add/delete (replace-all on Save chosen instead).
- Year field / age calculation.
- Day-of-month validation per month (1–31 accepted uniformly, matching current behaviour).
