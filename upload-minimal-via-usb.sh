#!/bin/bash
# Minimalen OTA-Test-Sketch per USB auf den ESP8266 flashen.
# Danach: ./upload-minimal-ota.sh stairlight-testbed.local --debug zum OTA-Test.
# Nutzung: ./upload-minimal-via-usb.sh [PORT]

set -e
cd "$(dirname "$0")"
PORT="${1:-/dev/cu.usbserial-0001}"

echo "→ Compile minimal_ota..."
arduino-cli compile --fqbn esp8266:esp8266:nodemcu minimal_ota

echo "→ Upload auf $PORT..."
arduino-cli upload -p "$PORT" --fqbn esp8266:esp8266:nodemcu minimal_ota

echo "→ Fertig. ESP startet mit Minimal-Sketch. OTA testen mit: ./upload-minimal-ota.sh stairlight-testbed.local --debug"
