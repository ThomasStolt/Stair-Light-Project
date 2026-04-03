# Changelog

## 1.0.0 – 2026-04-04

- **Night mode indicator** – Web UI shows a red badge when night mode is active (hours displayed).
- **Firmware version** – Version number (`FW_VERSION`) shown at the bottom of the web UI.
- **Night mode API** – `/api/state` now includes a `night` field (1 = active, 0 = inactive).
- Updated night mode defaults: 1:00–6:00, brightness max 50, min 10.

## Pre-1.0 (October 2016 – March 2026)

Initial development: animated stair lighting with SK6812 RGBW LEDs, PIR motion detection, OTA updates, web UI with manual colour control, stair automation toggle, animation test, night mode (red breathing), birthday animation, NTP time, reboot button, motion log, memory/CPU/WiFi status tables, All presets, firewall workaround scripts.
