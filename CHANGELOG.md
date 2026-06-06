# Changelog

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
