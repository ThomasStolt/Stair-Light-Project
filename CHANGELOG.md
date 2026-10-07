# Changelog

## 2.8.1 – 2026-10-08

- **Matrix runs 60 s from the web UI** (`MATRIX_WEB_MS`); the other animations still run 10 s there, and motion-triggered Matrix still runs 20 s.
- **Matrix tile looks like the film** – The web UI preview now shows falling columns of green katakana and digits with bright leading characters that keep changing, instead of plain green stripes.
- **Shorter, softer startup signal** – After a restart the stairs show three soft pulses (green with a little white) that fade in and out, 2 s in total and very dim, instead of three hard 1 s green blinks at 50 % (6 s).

## 2.8.0 – 2026-10-05

- **Rainbow reworked** – The rainbow now fades in as a smooth wave in walking direction: each step brightens over ~1 s and the next one starts when the previous is at 20 % (200 ms later; `RAINBOW_FADE_MS`, `RAINBOW_STAGGER_MS`), so the whole staircase is lit after ~4 s. The colours already flow while the steps fade in and out, and fade-in, animation and fade-out are drawn by the same code, so there is no visible colour or brightness jump between them. It runs for the full `ANIM_DURATION` (20 s, including the fade-in) like the other animations – previously it stopped after two colour cycles (~5 s down / ~10 s up). Up and down flow at the same speed (`RAINBOW_CYCLE_MS`, one colour cycle per 5 s), colours travelling in walking direction; fade-out is a quicker wave (~1.5 s). External control still interrupts it (it fades out from wherever it is).
- **Smoother fades via dithering** – Brightness and hue are computed at 256x finer resolution, and in-between levels are shown by alternating between neighbouring levels from frame to frame (all LEDs of a step stay identical). Fades no longer start in visible jumps at the dark end. `RAINBOW_DITHER 0` turns it off for comparison.
- **Star sparkle: real twinkling stars** – Stars used to flash for a single frame (~17 ms) at full brightness. Now each star switches on at a random brightness (`STAR_MIN_PEAK` 25 %–100 %) and fades out calmly over 0.75–1.5 s (`STAR_FADE_MS`), with ~20 new stars per second (`STAR_RATE`). The blue backdrop fades in and out as a wave like the night animation (1.5 s per step, next step starting at 10 %, ~3.7 s in total; `WAVE_FADE_MS`, `WAVE_STAGGER_MS`), and stars dim along with their step during the fades.
- **Birthday: calm confetti** – Instead of giving all 432 LEDs a new random colour every frame (and switching on/off instantly), about three quarters of the LEDs (`BDAY_ON_PCT` 75 %) now glow in random colours at random brightness, each fading in and out over 2–4 s (`BDAY_LIFE_MS`); when one goes dark another random LED takes over. It fades in and out as the same wave as the star sparkle (`WAVE_FADE_MS`, `WAVE_STAGGER_MS`).
- **New animation: Matrix** – "The Matrix" digital rain: each of the 27 LED positions is a column, and green drops with a white-green head and fading trail fall down the stairs at different speeds (`MATRIX_DROP_SPEED`, `MATRIX_TRAIL`), with trail LEDs flickering now and then. A faint green glow (`MATRIX_BASE`, 0 = black) keeps the steps visible. It fades in and out as the same wave as the star sparkle, joins the random motion rotation and has its own tile in the web UI (`/api/play anim=7`).
- **Random colour animation removed** – Motion now picks from Rainbow, White ramp, Star sparkle and Matrix (no back-to-back repeat). `/api/play` accepts `anim=2..7`.
- **New web UI** – Redesigned page: animation tiles with live previews (the tile of the animation that is playing, from the web or from motion, shows a progress ring), fixed colour with RGBW sliders and presets, recent motion as a timeline, device status with memory, flash and WiFi at a glance plus a full detail table, and settings, birthdays and restart (with an in-page confirmation). Form fields are 16 px so iOS does not zoom in. The page lives in `web_ui.h`; fonts load from Google Fonts when online, otherwise system fonts are used.
- **`/api/state` reports the running animation** – New fields `anim` (0 = none, 2–7), and while one runs `anim_ms`, `anim_len` and `dir`. All animations now start through one `runAnimation()` function.
- **Fix: night-mode motions showed no animation name** in the motion log (`/api/log` now returns "Night red").
- **Fix: red and green were swapped in the colour helpers** – `red()`/`green()` in `parking.h` read the wrong byte, which made the old rainbow fade in with red and green exchanged and then jump to the correct colours.
- **Automation switch survives restarts** – Turning stair automation off (web UI or `/api/auto`) is now saved, so the device comes back the same way after a restart or power cut. Existing devices start with automation on, as before. This makes the firmware identical for every device (only `credentials.h` differs), so one build can be uploaded to several devices.

## 2.7.0 – 2026-06-08

- **External control now preempts a running animation** – A `POST /api/ext` command (red/red_blink/yellow_blink/green_fade/clean/clear) now interrupts an in-progress motion animation instead of waiting for it (and its 10 s post-delay) to finish. The animation hold-phases and the post-animation delay abort as soon as a command is pending, so the strip switches to the requested state within ~1–2 s instead of up to ~30 s.

## 2.6.1 – 2026-06-08

- **Docs** – Added a German Siri/Kurzbefehle setup guide to the README. Documentation only; no firmware behaviour change vs 2.6.0.

## 2.6.0 – 2026-06-07

- **Cleaning light fades in/out** – `state=clean` now ramps up smoothly from 0 to 250 over ~500 ms instead of switching on instantly, and fades back down 250→0 over ~500 ms when it ends (on `clear`/"Staubsaugen aus" and on the 10-minute auto-timeout).

## 2.5.2 – 2026-06-07

- **Fix: no spurious animation after an external override ends** – After `clear` (e.g. "Staubsaugen aus"), a green_fade finishing, or a hold timeout, the firmware now ignores the PIR sensors until they next read idle. Previously the lingering presence from the cleaning session (PIR still HIGH) immediately triggered an animation. A genuinely new motion still triggers normally.

## 2.5.1 – 2026-06-07

- **Cleaning light brightness** – `state=clean` now drives all channels to **250** (instead of 255) to leave a little headroom on power/drivers.

## 2.5.0 – 2026-06-07

- **Cleaning light ("Staubsaugen")** – `POST /api/ext state=clean` turns every LED to full brightness (all channels 255) for vacuuming/cleaning, held for up to **10 minutes** or until `state=clear` ("Staubsaugen aus"). Re-sending `clean` resets the 10-minute timer. Intended to be triggered by an Apple Shortcut via Siri ("Hey Siri, Staubsaugen!"). Note: all-channels-255 across 432 LEDs is a heavy sustained current draw — ensure the PSU/wiring can handle it.

## 2.4.0 – 2026-06-07

- **External control safety timeout** – Held states (`red`, `red_blink`, `yellow_blink`) now auto-release after **5 minutes** if no new `/api/ext` command arrives, returning to normal operation (motion automation / night mode). Each command resets the timer. `green_fade` already self-terminates (~30 s) and is unaffected. This prevents the stairs getting stuck if the controlling process crashes or loses WiFi.

## 2.3.0 – 2026-06-07

- **External control: blinking yellow** – `POST /api/ext` now accepts `state=yellow_blink`, which blinks the strip yellow (red + green) every 500 ms until the next command. Like the other external states, it suppresses motion detection while active.

## 2.2.1 – 2026-06-07

- **Night mode across midnight** – Fix: a night window where the start hour is later than the end hour (e.g. 23–6) now works. Previously only same-day windows (start < end) activated.
- **Birthday table labels** – The Birthdays editor now shows Month / Day / Name column headers and `MM`/`DD` placeholders so it's clear which field is which.

## 2.2.0 – 2026-06-07

- **Web-editable birthdays** – View and edit the birthday list (month, day, optional name) in the web UI under "Birthdays"; add/remove rows and Save. Each birthday can have a short name shown in the UI.
- **Persistent birthdays** – Birthdays are stored in a dedicated EEPROM region (separate from settings) and survive reboot; `birthdays.h` is now only the first-boot default. Up to 20 entries.
- **New endpoints** – `GET`/`POST /api/birthdays`.

## 2.1.0 – 2026-06-06

- **External control API** – `POST /api/ext` with `state=red|red_blink|green_fade|clear` lets another LAN process drive the strip directly (solid red, 500 ms red blink, ~30 s green dim-down). Motion detection is suppressed while active; normal behaviour (or night mode) resumes after `green_fade`/`clear`.
- **Web-configurable settings** – Hostname and night mode (enable toggle + start/end hours) are now editable in the web UI under "Settings".
- **Persistent settings** – Hostname and night-mode settings are stored in EEPROM and survive reboot (fall back to compiled defaults on first boot). Hostname changes apply after reboot.
- **New endpoints** – `GET`/`POST /api/settings`.

## 1.0.0 – 2026-04-04

- **Night mode indicator** – Web UI shows a red badge when night mode is active (hours displayed).
- **Firmware version** – Version number (`FW_VERSION`) shown at the bottom of the web UI.
- **Night mode API** – `/api/state` now includes a `night` field (1 = active, 0 = inactive).
- Updated night mode defaults: 1:00–6:00, brightness max 50, min 10.

## Pre-1.0 (October 2016 – March 2026)

Initial development: animated stair lighting with SK6812 RGBW LEDs, PIR motion detection, OTA updates, web UI with manual colour control, stair automation toggle, animation test, night mode (red breathing), birthday animation, NTP time, reboot button, motion log, memory/CPU/WiFi status tables, All presets, firewall workaround scripts.
