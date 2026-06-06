# Web-Editable Birthdays Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the user view and edit the birthday list (month, day, optional name) from the Web UI, persisted to a dedicated EEPROM region, with `birthdays.h` as the first-boot default.

**Architecture:** Single Arduino sketch. A new `BirthdayStore` struct lives in its own EEPROM region (offset 128), independent of the existing `Settings` region (offset 0) so saved settings survive the upgrade. Async handlers validate + persist; `isBirthdayToday()` reads the runtime copy. New `GET/POST /api/birthdays` and a Web UI "Birthdays" section.

**Tech Stack:** ESP8266 Arduino core, ESPAsyncWebServer, `<EEPROM.h>`, arduino-cli.

---

## Testing approach (read first)

No unit-test harness (firmware, can't run off-device). Each task's gate is a **clean compile**:
```
cd "/Users/tstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project" && "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light
```
A clean compile ends with memory-usage tables and no error lines. The baseline compiles cleanly today, so any error after an edit is from that edit. On-device `curl`/browser checks are done by the human after merge. Commit only the files named in each task — the working tree has unrelated `build_minimal/` artifacts; never stage those.

---

## File structure

- **Modify** `rgbw_stair_light/rgbw_stair_light.ino` — all firmware changes (EEPROM region + struct, persistence fns, `isBirthdayToday()` wiring, two API handlers + routes, Web UI section + JS, version bump). Single-sketch project; follows the established pattern.
- **Modify** `CHANGELOG.md`, `README.md` — docs.
- `rgbw_stair_light/birthdays.h` — unchanged; retained as the default seed (gitignored).

---

## Task 1: Birthday EEPROM region + persistence

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (EEPROM size/offset defines; widen existing `EEPROM.begin` calls; `BirthdayStore` struct + load/save; call `loadBirthdays()` in `setup()`)

- [ ] **Step 1: Add EEPROM layout defines before the Settings block**

Find this line (currently ~183):
```cpp
#define SETTINGS_MAGIC   0x53544C31u  // 'STL1'
```
Insert immediately BEFORE it:
```cpp
// EEPROM is shared: Settings at offset 0, birthdays at a fixed offset further in.
#define EEPROM_SIZE            1024
#define EEPROM_BIRTHDAYS_ADDR  128   // Settings live at offset 0; leave headroom to grow
```

- [ ] **Step 2: Widen the two existing `EEPROM.begin` calls**

In `saveSettings()` change:
```cpp
  EEPROM.begin(sizeof(Settings));   // idempotent; makes saveSettings() safe to call standalone
```
to:
```cpp
  EEPROM.begin(EEPROM_SIZE);   // idempotent; makes saveSettings() safe to call standalone
```
In `loadSettings()` change:
```cpp
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, g_settings);
```
to:
```cpp
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, g_settings);
```

- [ ] **Step 3: Add the BirthdayStore struct + load/save functions**

Find the end of `loadSettings()` — the closing brace followed by a blank line, just before the `WiFiUDP udp;` line (currently ~216). Insert AFTER `loadSettings()`'s closing brace:
```cpp
// ---- Persistent birthdays (separate EEPROM region) ------------------------
// Independent magic/version so it self-heals without touching the Settings
// region. Seeded from the compile-time birthdays.h list on first boot.
#define BIRTHDAYS_MAX     20
#define BIRTHDAY_NAME_LEN 20            // includes null terminator (<=19 visible chars)
#define BIRTHDAYS_MAGIC   0x53544231u   // 'STB1'
#define BIRTHDAYS_VERSION 1
struct BirthdayStore {
  uint32_t magic;
  uint8_t  version;
  uint8_t  count;                       // 0..BIRTHDAYS_MAX
  struct {
    uint8_t month;                      // 1..12
    uint8_t day;                        // 1..31
    char    name[BIRTHDAY_NAME_LEN];    // null-terminated, may be empty
  } items[BIRTHDAYS_MAX];
};
BirthdayStore g_birthdays;

void saveBirthdays() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(EEPROM_BIRTHDAYS_ADDR, g_birthdays);
  EEPROM.commit();
}

void loadBirthdays() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_BIRTHDAYS_ADDR, g_birthdays);
  if (g_birthdays.magic != BIRTHDAYS_MAGIC || g_birthdays.version != BIRTHDAYS_VERSION) {
    // First boot / blank / schema change → seed from compiled birthdays.h defaults
    g_birthdays.magic   = BIRTHDAYS_MAGIC;
    g_birthdays.version = BIRTHDAYS_VERSION;
    uint8_t n = (BIRTHDAY_COUNT > BIRTHDAYS_MAX) ? BIRTHDAYS_MAX : (uint8_t)BIRTHDAY_COUNT;
    g_birthdays.count = n;
    for (uint8_t i = 0; i < n; i++) {
      g_birthdays.items[i].month   = BIRTHDAYS[i][0];
      g_birthdays.items[i].day     = BIRTHDAYS[i][1];
      g_birthdays.items[i].name[0] = '\0';
    }
    saveBirthdays();
  }
}
```

- [ ] **Step 4: Call `loadBirthdays()` in `setup()`**

Find this line in `setup()` (currently ~853):
```cpp
  loadSettings();
```
Change it to:
```cpp
  loadSettings();
  loadBirthdays();
```

- [ ] **Step 5: Compile-verify**

Run the compile command. Expected: clean (new symbols unused so far is fine; Task 2 wires `isBirthdayToday()`).

- [ ] **Step 6: Commit**

```bash
cd "/Users/tstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project"
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add EEPROM-backed birthday store (separate region, seeded from birthdays.h)"
```

---

## Task 2: Drive isBirthdayToday() from the runtime store

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (`isBirthdayToday()`)

- [ ] **Step 1: Replace the function body**

Replace the whole `isBirthdayToday()` function (currently ~672–684) with:
```cpp
bool isBirthdayToday(long unixTimeUtc) {
  if (g_birthdays.count == 0) return false;
  time_t t = (time_t)(unixTimeUtc + TIMEZONE_OFFSET_SEC(unixTimeUtc));
  struct tm *tm = gmtime(&t);
  if (!tm) return false;
  int month = tm->tm_mon + 1;
  int day = tm->tm_mday;
  for (uint8_t i = 0; i < g_birthdays.count; i++) {
    if ((int)g_birthdays.items[i].month == month && (int)g_birthdays.items[i].day == day)
      return true;
  }
  return false;
}
```

- [ ] **Step 2: Compile-verify**

Run the compile command. Expected: clean.

- [ ] **Step 3: Commit**

```bash
cd "/Users/tstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project"
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Use runtime birthday store in isBirthdayToday()"
```

---

## Task 3: GET + POST /api/birthdays

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (two handlers + two routes)

- [ ] **Step 1: Add both handlers**

Add immediately AFTER `handleApiSettingsPost(...)`'s closing brace (the function currently starts ~728; insert after its final `}`):
```cpp
// GET /api/birthdays – JSON array of {m,d,name} for the Web UI
void handleApiBirthdaysGet(AsyncWebServerRequest *request) {
  String json = "[";
  for (uint8_t i = 0; i < g_birthdays.count; i++) {
    if (i > 0) json += ",";
    String nm = g_birthdays.items[i].name;
    nm.replace("\"", "'");
    json += "{\"m\":"; json += g_birthdays.items[i].month;
    json += ",\"d\":"; json += g_birthdays.items[i].day;
    json += ",\"name\":\""; json += nm; json += "\"}";
  }
  json += "]";
  AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), json);
  response->addHeader(F("Cache-Control"), F("no-store"));
  request->send(response);
}

// POST /api/birthdays – replace the whole list.
// Body (form-encoded): count=N then for i in 0..N-1: m<i>, d<i>, n<i>
void handleApiBirthdaysPost(AsyncWebServerRequest *request) {
  if (!request->hasParam(F("count"), true)) {
    request->send(400, F("text/plain"), F("count required")); return;
  }
  int count = request->getParam(F("count"), true)->value().toInt();
  if (count < 0 || count > BIRTHDAYS_MAX) {
    request->send(400, F("text/plain"), F("count 0..20")); return;
  }
  // Build into a static temp; only commit to g_birthdays after all entries validate
  // (no partial write). static avoids ~450 B on the async callback stack.
  static BirthdayStore tmp;
  tmp.magic   = BIRTHDAYS_MAGIC;
  tmp.version = BIRTHDAYS_VERSION;
  tmp.count   = (uint8_t)count;
  for (int i = 0; i < count; i++) {
    String mi = String("m") + i, di = String("d") + i, ni = String("n") + i;
    if (!request->hasParam(mi, true) || !request->hasParam(di, true)) {
      request->send(400, F("text/plain"), F("missing m/d for an entry")); return;
    }
    int m = request->getParam(mi, true)->value().toInt();
    int d = request->getParam(di, true)->value().toInt();
    if (m < 1 || m > 12) { request->send(400, F("text/plain"), F("month 1..12")); return; }
    if (d < 1 || d > 31) { request->send(400, F("text/plain"), F("day 1..31")); return; }
    String nm = request->hasParam(ni, true) ? request->getParam(ni, true)->value() : String("");
    nm.trim();
    if (nm.length() > BIRTHDAY_NAME_LEN - 1) {
      request->send(400, F("text/plain"), F("name too long (max 19)")); return;
    }
    for (size_t k = 0; k < nm.length(); k++) {
      char c = nm[k];
      bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_' || c == '.';
      if (!ok) { request->send(400, F("text/plain"), F("name: letters/digits/space/-_. only")); return; }
    }
    tmp.items[i].month = (uint8_t)m;
    tmp.items[i].day   = (uint8_t)d;
    strncpy(tmp.items[i].name, nm.c_str(), BIRTHDAY_NAME_LEN - 1);
    tmp.items[i].name[BIRTHDAY_NAME_LEN - 1] = '\0';
  }
  g_birthdays = tmp;
  saveBirthdays();
  request->send(204);
}
```

- [ ] **Step 2: Register the routes**

Find these lines in `setup()` (currently ~952–953):
```cpp
  server.on("/api/settings", HTTP_GET,  handleApiSettingsGet);
  server.on("/api/settings", HTTP_POST, handleApiSettingsPost);
```
Insert immediately AFTER them:
```cpp
  server.on("/api/birthdays", HTTP_GET,  handleApiBirthdaysGet);
  server.on("/api/birthdays", HTTP_POST, handleApiBirthdaysPost);
```

- [ ] **Step 3: Compile-verify**

Run the compile command. Expected: clean.

- [ ] **Step 4: Commit**

```bash
cd "/Users/tstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project"
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add GET/POST /api/birthdays (replace-all, validated, persisted)"
```

---

## Task 4: Web UI Birthdays section

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (`handleIndex()` PROGMEM HTML + JS)

Reminder: each inserted line is a separate quoted C string concatenated by the compiler. Any `"` inside the HTML/JS must be escaped `\"`; single-quoted JS strings need no escaping. Compile after each step to catch a broken literal early. The Add/Save buttons use `class="btn preset"`, which is safe — the color-preset click handler is scoped to `.preset[data-all]` and these have no `data-all`.

- [ ] **Step 1: Add the Birthdays section HTML**

Find the firmware-version footer line (currently ~344):
```cpp
    "<p style=margin-top:1rem;font-size:0.75rem;color:#666;>Firmware v" FW_VERSION "</p></main>"
```
Insert the following block IMMEDIATELY BEFORE that footer line:
```cpp
    "<details id=detailBirthdays><summary>Birthdays</summary><div class=inner>"
    "<div id=bdayRows></div>"
    "<div class=row style=justify-content:flex-start;margin-top:0.5rem;>"
    "<button type=button class=\"btn preset\" id=bdayAdd>+ Add</button>"
    "<button type=button class=\"btn preset\" id=bdaySave>Save</button>"
    "<span id=bdayStatus style=font-size:0.85rem;></span></div>"
    "</div></details>"
```
Compile.

- [ ] **Step 2: Add the Birthdays JavaScript**

Find the init line (currently ~418):
```cpp
    "loadFast();loadSlow();loadSettings();"
```
Insert the following block IMMEDIATELY BEFORE that init line:
```cpp
    "var BDAY_MAX=20;"
    "function bdayRow(m,d,name){"
    "var div=document.createElement('div');div.className='slider-row';div.style.gap='6px';"
    "div.innerHTML='<input type=number min=1 max=12 class=bm style=\"width:3.2em;background:#111;color:#eee;border:1px solid #444;border-radius:6px;padding:0.3rem;\"> '"
    "+'<input type=number min=1 max=31 class=bd style=\"width:3.2em;background:#111;color:#eee;border:1px solid #444;border-radius:6px;padding:0.3rem;\"> '"
    "+'<input type=text class=bn placeholder=name style=\"flex:1;min-width:0;background:#111;color:#eee;border:1px solid #444;border-radius:6px;padding:0.3rem;\"> '"
    "+'<button type=button class=btn style=\"padding:0.2rem 0.6rem;background:#522;color:#f99;\">x</button>';"
    "div.querySelector('.bm').value=m;div.querySelector('.bd').value=d;div.querySelector('.bn').value=name||'';"
    "div.querySelector('button').onclick=function(){div.remove();};"
    "return div;}"
    "function loadBirthdays(){"
    "fetch('/api/birthdays').then(function(r){return r.json();}).then(function(a){"
    "var box=document.getElementById('bdayRows');box.innerHTML='';"
    "for(var i=0;i<a.length;i++){box.appendChild(bdayRow(a[i].m,a[i].d,a[i].name));}"
    "});}"
    "document.getElementById('bdayAdd').onclick=function(){"
    "var box=document.getElementById('bdayRows');if(box.children.length>=BDAY_MAX)return;box.appendChild(bdayRow('','',''));};"
    "document.getElementById('bdaySave').onclick=function(){"
    "var rows=document.getElementById('bdayRows').children;var body='count='+rows.length;"
    "for(var i=0;i<rows.length;i++){"
    "body+='&m'+i+'='+encodeURIComponent(rows[i].querySelector('.bm').value)"
    "+'&d'+i+'='+encodeURIComponent(rows[i].querySelector('.bd').value)"
    "+'&n'+i+'='+encodeURIComponent(rows[i].querySelector('.bn').value);}"
    "var st=document.getElementById('bdayStatus');st.textContent='Saving...';st.style.color='#fc8';"
    "post('/api/birthdays',body).then(function(r){"
    "if(r.status===204){st.textContent='Saved';st.style.color='#9f9';loadBirthdays();}"
    "else{st.textContent='Invalid';st.style.color='#f88';}"
    "});};"
```
Compile.

- [ ] **Step 3: Call `loadBirthdays()` on page load**

Change the init line from:
```cpp
    "loadFast();loadSlow();loadSettings();"
```
to:
```cpp
    "loadFast();loadSlow();loadSettings();loadBirthdays();"
```

- [ ] **Step 4: Compile-verify**

Run the compile command. Expected: clean.

- [ ] **Step 5: Commit**

```bash
cd "/Users/tstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project"
git add rgbw_stair_light/rgbw_stair_light.ino
git commit -m "Add web UI Birthdays section (list/add/remove/save)"
```

---

## Task 5: Version bump + documentation

**Files:**
- Modify: `rgbw_stair_light/rgbw_stair_light.ino` (`FW_VERSION`), `CHANGELOG.md`, `README.md`

- [ ] **Step 1: Bump the firmware version**

Change (currently ~169):
```cpp
#define FW_VERSION "2.1.0"
```
to:
```cpp
#define FW_VERSION "2.2.0"
```

- [ ] **Step 2: Add a CHANGELOG entry**

Insert at the top of `CHANGELOG.md`, directly under the `# Changelog` heading (newest entry), with a blank line after it:
```markdown
## 2.2.0 – 2026-06-07

- **Web-editable birthdays** – View and edit the birthday list (month, day, optional name) in the web UI under "Birthdays"; add/remove rows and Save. Each birthday can have a short name shown in the UI.
- **Persistent birthdays** – Birthdays are stored in a dedicated EEPROM region (separate from settings) and survive reboot; `birthdays.h` is now only the first-boot default. Up to 20 entries.
- **New endpoints** – `GET`/`POST /api/birthdays`.
```

- [ ] **Step 3: Document in README.md**

In `README.md`, find the `## External control API` section heading (added in 2.1.0). Insert the following NEW section immediately BEFORE it (leave a blank line after the new section, before `## External control API`):
```markdown
## Birthdays

Birthdays (month, day, and an optional name) trigger the birthday animation on the day.
They are editable in the web UI under **Birthdays** — add or remove rows and click Save —
and are stored in flash (up to 20 entries), surviving reboots. `birthdays.h` is only the
first-boot default; after that the saved list is authoritative.

API:

```bash
curl http://<host>/api/birthdays
# [{"m":11,"d":19,"name":"Anna"}, ...]
```

`POST /api/birthdays` replaces the whole list (form-encoded: `count=N` then `m<i>`,
`d<i>`, `n<i>` for each entry). Months 1–12, days 1–31, names up to 19 characters
(letters, digits, space, `-` `_` `.`).

```

- [ ] **Step 4: Compile-verify**

Run the compile command. Expected: clean (footer now v2.2.0).

- [ ] **Step 5: Commit**

```bash
cd "/Users/tstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project"
git add rgbw_stair_light/rgbw_stair_light.ino CHANGELOG.md README.md
git commit -m "Bump to 2.2.0; document web-editable birthdays"
```

---

## Self-review checklist (completed by plan author)

- **Spec coverage:** separate EEPROM region @128 + EEPROM_SIZE widen (Task 1) ✓; BirthdayStore struct with name[20], max 20, magic/version self-heal seeded from birthdays.h (Task 1) ✓; isBirthdayToday uses g_birthdays (Task 2) ✓; GET returns {m,d,name} sanitised + no-store (Task 3) ✓; POST replace-all, indexed params, validation (count 0–20, month 1–12, day 1–31, name ≤19 + charset), validate-then-commit no partial write (Task 3) ✓; routes registered (Task 3) ✓; Web UI list/add/remove/save section + JS + load on page load (Task 4) ✓; FW_VERSION 2.2.0 + docs (Task 5) ✓; settings region untouched (Settings struct/magic/version unchanged; only begin size widened) ✓; name UI-only, animation unchanged ✓.
- **Placeholder scan:** none — all steps contain concrete code/commands.
- **Type/name consistency:** `g_birthdays`, `BirthdayStore`, `BIRTHDAYS_MAX`, `BIRTHDAY_NAME_LEN`, `BIRTHDAYS_MAGIC`/`BIRTHDAYS_VERSION`, `EEPROM_SIZE`, `EEPROM_BIRTHDAYS_ADDR`, `loadBirthdays`/`saveBirthdays`, `handleApiBirthdaysGet`/`handleApiBirthdaysPost` used consistently; JSON keys `m`/`d`/`name` match between GET output, JS reader, and POST param names `m<i>`/`d<i>`/`n<i>`; reuses existing `setAll`/`post()`/`.slider-row`/`.preset[data-all]` correctly.
```
