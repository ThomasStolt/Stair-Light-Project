# Stair Light Project

Automatic animated stair lighting with **SK6812 RGBW** LEDs. Two PIR sensors (SR-HC501) at the first and last step detect direction and trigger a random animation. Control via **web UI** (stair automation on/off, manual colours, night mode indicator, 10 s animation test) and optional **night mode** (1–6 h: red only, breathing).

- **MCU:** ESP8266 (e.g. NodeMCU)
- **LEDs:** SK6812 RGBW (WS2812-compatible), 27 LEDs per step, 16 steps (configurable)
- **Sensors:** PIR1 = “up”, PIR2 = “down”
- **Boot:** Stair automation is **on** after boot; 3× green blink indicates “ready”.

<div align="center">
  <img src="images/stairlight-neopixel.gif" alt="Stair Light test bed" width="100%">
</div>


---

## Requirements

- **Arduino CLI** (e.g. `brew install arduino-cli`)
- **ESP8266 core:**  
  `arduino-cli core update-index && arduino-cli core install esp8266:esp8266`
- **Libraries:**  
  `arduino-cli lib install "Adafruit NeoPixel"`  
  `arduino-cli lib install "EasyNTPClient"`

---

## Project structure

| Folder/File | Description |
|-------------|-------------|
| `rgbw_stair_light/` | Main sketch (stair light + OTA) |
| `rgbw_stair_light/parking.h` | Animations and helpers |
| `rgbw_stair_light/birthdays.h.example` | Template for birthdays (copy to `birthdays.h`) |
| `schematics/` | KiCad schematic, PCB |
| `images/` | Photos (incl. test bed) |

**Scripts (project root):**

| Script | Purpose |
|--------|---------|
| `upload-to-esp8266.sh` | Compile + upload **via USB** |
| `upload-to-esp8266-ota.sh` | Compile + upload **via OTA** (WiFi) |
| `upload-ota-firewall-ok.sh` | OTA with **firewall temporarily disabled** (macOS) |
| `setup-firewall-ota.sh` | Add firewall rule for OTA (macOS, sudo) |

---

## Setup

### 1. WiFi and hostname

Credentials go in **`rgbw_stair_light/credentials.h`** (gitignored).

- Copy `credentials.h.example` to `credentials.h` and set:
  - `WIFI_SSID` – your Wi‑Fi name  
  - `WIFI_PASS` – Wi‑Fi password  
  - `OTA_HOSTNAME` – e.g. `stairlight-testbed` or `stairlight`

Without `credentials.h` the build fails.

> The hostname can also be changed at runtime in the web UI (**Settings** section); it is stored in flash and applied on the next reboot. `OTA_HOSTNAME` in `credentials.h` is only the default used on first boot.

### 2. Birthdays (optional)

On dates listed in **`rgbw_stair_light/birthdays.h`**, only the birthday animation runs.

- **`birthdays.h`** is in `.gitignore` and is not committed.
- Copy `birthdays.h.example` to `birthdays.h`, set `BIRTHDAY_COUNT` and the `BIRTHDAY(month, day)` list (e.g. `BIRTHDAY(3, 15)` = 15 March).
- Without `birthdays.h` the build fails – after cloning, copy the example (use 0 entries if you don’t need birthdays).

### 3. Firewall (macOS, only if OTA fails)

If OTA fails with the firewall on:

- **Option A – rule via CLI:**  
  `sudo ./setup-firewall-ota.sh`  
  (allows Arduino Python and Terminal/Cursor to accept incoming connections)

- **Option B – OTA with firewall briefly off:**  
  `./upload-ota-firewall-ok.sh stairlight-testbed.local`  
  (firewall is disabled only during the upload, then re-enabled)

---

## First upload: via USB

1. Connect the ESP8266 via USB.
2. Check port (optional):  
   `arduino-cli board list`
3. From the project folder run:

   ```bash
   ./upload-to-esp8266.sh
   ```

   Default port is `/dev/cu.usbserial-0001`. To use another port:

   ```bash
   ./upload-to-esp8266.sh /dev/cu.wchusbserial-12345
   ```

4. After a successful upload the ESP connects to Wi‑Fi and is ready for OTA.

---

## Upload via OTA (WiFi)

Prerequisite: the ESP is already running firmware with **ArduinoOTA** (e.g. after the first USB upload) and is on the same Wi‑Fi network. The OTA script uses **espota.py** (no board discovery required); pass the ESP’s IP or mDNS hostname.

- **By hostname (mDNS):**  
  ```bash
  ./upload-to-esp8266-ota.sh stairlight-testbed.local
  ```

- **By IP:**  
  ```bash
  ./upload-to-esp8266-ota.sh 192.168.2.185
  ```

The IP is shown in the serial monitor after “Connected, IP address:” or “Web server: http://…”.

**Web UI:** Open `http://<IP>` or `http://<OTA_HOSTNAME>.local` in a browser.

- **Date & time** – NTP-synced local time (updates every second).
- **Stair automation** on/off (**on** by default after boot).
- **Manual colours** – Per channel (red, green, blue, white): −10%, on/off, +10% brightness; value shown in %.
- **All** – Preset buttons 0%, 25%, 50%, 75%, 100% to set all channels to the same brightness.
- **Reboot** – Restart the ESP from the browser.
- **Last 5 motions** – Table of recent PIR triggers: time, direction (up/down), animation started.
- **Memory status** – Table: Heap (RAM), Flash, RTC with total, used, and usage %.
- **Night mode indicator** – Red badge shown when night mode is active (with hours displayed).
- **Animation (10 s)** – Dropdown with Random fade, Rainbow, White ramp, Star sparkle, Birthday, **Night (red breathing)**; **Go** runs the selected animation for 10 seconds (any time of day).
- **Firmware version** – Shown at the bottom of the page.

The frontend is cached by the browser; actions use the API (GET state/time/log/memory, POST for actions). Web server and OTA stay available during animations.

**If the firewall (macOS) blocks OTA:**  
Use the wrapper script instead of `upload-to-esp8266-ota.sh`:

```bash
./upload-ota-firewall-ok.sh stairlight-testbed.local
```

The firewall is then disabled only during the upload and re-enabled afterwards.

**espota debug output:**  
```bash
./upload-to-esp8266-ota.sh stairlight-testbed.local --debug
```

---

## Serial monitor

Baud rate: **115200**.

- **Via task (Cursor/VS Code):**  
  `Cmd+Shift+P` → “Tasks: Run Task” → “Serial Monitor (115200 Baud)” → enter port.

- **Via terminal:**  
  ```bash
  arduino-cli monitor -p /dev/cu.usbserial-0001 -c baudrate=115200
  ```

Adjust the port if needed (`arduino-cli board list`). Exit with `Ctrl+C`. While the monitor is running the USB port is in use (no upload).

**Note:** With `USE_SERIAL.setDebugOutput(true)` in the sketch, the ESP8266 WiFi library also prints connection messages (`state:`, `reconnect`, `scandone`, etc.). For a quieter monitor you can set `setDebugOutput(false)` to see only PIR/NTP and your own messages.

---

## Night mode (1–6 h)

Between **1:00 and 6:00 local time** (NTP + auto CET/CEST):

- Only the **night animation** runs: soft breathing red, max 20% brightness, **never fully off** (minimum brightness is configurable).
- PIR triggers the night animation; manual web colours have no effect.
- After 6:00 the strip turns off and normal behaviour (automation/manual) applies again.
- The web UI shows a **red "Night mode active" badge** when night mode is on.

The same “Night (red breathing)” animation can be tested anytime in the web UI under **Animation (10 s)** → **Go** for 10 seconds.

---

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

## External control API

Another process on the same network can drive the strip directly, bypassing motion
detection — e.g. a parking/garage helper signalling stop/go.

`POST /api/ext` with form field `state`:

| `state`      | Effect                                                        |
|--------------|---------------------------------------------------------------|
| `red`        | Solid red, held until the next command                        |
| `red_blink`    | Red blinking every 500 ms                                   |
| `green_fade`   | Green dimming from full to off over ~30 s, then auto-clears |
| `yellow_blink` | Yellow (red + green) blinking every 500 ms                  |
| `clean`        | All LEDs fade up to near-max (R+G+B+W = 250) for cleaning; fades out on stop; auto-off after 10 min |
| `clear`        | LEDs off immediately, override released                     |

While a command is active, motion detection is suppressed. After `green_fade` finishes
(or `clear`), normal behaviour resumes — daytime automation, or night mode if within
the configured night hours. No authentication (trusted LAN only). An `/api/ext` command
also **interrupts a running motion animation** (it aborts within ~1–2 s rather than
waiting for the animation and its post-delay to finish).

The held states (`red`, `red_blink`, `yellow_blink`) stay active until the next command,
but have a **5-minute safety timeout**: if no new `/api/ext` command arrives within 5
minutes, the override releases automatically and normal operation resumes (so the stairs
can't get stuck if the controller crashes or loses WiFi). Each command resets the timer.
`green_fade` self-terminates after ~30 s.

```bash
curl -X POST -d state=red        http://<host>/api/ext
curl -X POST -d state=red_blink    http://<host>/api/ext
curl -X POST -d state=green_fade   http://<host>/api/ext
curl -X POST -d state=yellow_blink http://<host>/api/ext
curl -X POST -d state=clean        http://<host>/api/ext
curl -X POST -d state=clear        http://<host>/api/ext
```

### Siri / Apple Shortcuts ("Staubsaugen")

Siri can't call the ESP directly, but an Apple **Shortcut** can, and Siri runs Shortcuts by
name. Create two shortcuts in the Shortcuts app (iPhone/iPad/Mac):

1. **"Staubsaugen"** (cleaning light on):
   - Add action **Get Contents of URL**
   - URL: `http://stairlight.local/api/ext` (or the device IP, e.g. `http://<device-ip>/api/ext`)
   - Method: **POST**, Request Body: **Form**, add field `state` = `clean`
2. **"Staubsaugen aus"** (off):
   - Same action/URL, field `state` = `clear`

Then say **"Hey Siri, Staubsaugen!"** to turn all LEDs to full brightness (auto-off after
10 minutes) and **"Hey Siri, Staubsaugen aus!"** to turn it off. The device must be reachable
on the same network (mDNS name `stairlight.local`, or use a DHCP-reserved IP).

#### Siri / Kurzbefehle einrichten (Deutsch)

Siri kann den ESP nicht direkt ansprechen, aber Siri führt **Kurzbefehle** (Shortcuts)
per Namen aus. Lege dafür zwei Kurzbefehle in der **Kurzbefehle**-App (iPhone/iPad/Mac) an:

1. **Kurzbefehl „Staubsaugen"** (Licht an):
   - Aktion **„Inhalte von URL abrufen"** hinzufügen
   - URL: `http://stairlight.local/api/ext` (oder die Geräte-IP, z. B. `http://<device-ip>/api/ext`)
   - Auf **▸ Mehr anzeigen** tippen: **Methode** = `POST`, **Anfragetext** = `Formular`,
     Feld **`state`** = `clean`
2. **Kurzbefehl „Staubsaugen aus"** (Licht aus): gleich wie oben, aber Feld **`state`** = `clear`

Dann **„Hey Siri, Staubsaugen!"** schaltet alle LEDs auf volle Helligkeit (sanft hochdimmen,
automatische Abschaltung nach 10 Minuten) und **„Hey Siri, Staubsaugen aus!"** dimmt wieder
herunter. Hinweise: Das iPhone muss im selben WLAN sein; beim ersten Mal fragt iOS nach der
Erlaubnis für das lokale Netzwerk → **Erlauben**. Falls `stairlight.local` nicht auflöst, die
IP verwenden (am besten im Router eine feste IP / DHCP-Reservierung vergeben).

Night mode (enable + start/end hours) and the hostname are configurable in the web UI
**Settings** section and persist across reboots.

---

## Sketch configuration

In **`rgbw_stair_light/rgbw_stair_light.ino`** (and `parking.h`):

| Constant | Meaning | Default |
|----------|---------|---------|
| `STEPS` | Number of steps | 16 |
| `WIDTH` | LEDs per step | 27 |
| `ANIM_DURATION` | Duration of a PIR-triggered animation (ms) | 20000 |
| `POST_ANIM_DELAY_MS` | Delay after animation before next trigger (ms) | 10000 |
| `BRIGHTNESS` | LED brightness 0–255 | 255 |
| `DEBUG` | 1 = PIR/trigger on serial monitor | 1 |
| `TIMEZONE_OFFSET_SEC` | Seconds UTC→local (e.g. 3600 for CET) | 3600 |
| `FW_VERSION` | Firmware version string shown in web UI | "2.1.0" |
| `NIGHT_HOUR_START` / `NIGHT_HOUR_END` | Night mode from hour … to (excl.) | 1, 6 |
| `NIGHT_BRIGHTNESS_MAX` / `NIGHT_BRIGHTNESS_MIN` | Red in night mode max/min (0–255) | 50, 10 |

Pins (see comments in sketch): NeoPixel = GPIO 14 (D5), PIR1 = GPIO 16 (D0), PIR2 = GPIO 4 (D2). Do not use GPIO 15 and 2 (boot behaviour).

---

## Animations

Direction (up/down) is detected via PIR1/PIR2. With stair automation on, a random one of these animations runs (no back-to-back repeat):

- **Random fade** (`simpleFadeToRandom`) – steps fade in/out in a random colour  
- **Rainbow** (`rainbowSteps`) – rainbow per step, then run  
- **White ramp** (`FadeToFullBrightness`) – all steps to white  
- **Star sparkle** (`starSparkle`) – dark blue background with white “stars”

On **birthdays** (from `birthdays.h`) only the **birthday animation** runs (random colours, 50% brightness).

In the web UI under **Animation (10 s)** you can test all of the above plus **Night (red breathing)** for 10 seconds with “Go”.

More functions and possible new animations are in **`parking.h`**.

---

## License / contact

Originally from October 2016; sketch and structure have been extended (OTA, credentials, web API, night mode, birthdays, firewall workaround, date/time, reboot, motion log, memory status table, All presets, automation on by default, OTA via espota.py, night mode indicator, firmware version display).

For questions or ideas for new animations, contact the repository owner.
