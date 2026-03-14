#!/bin/bash
# Upload des Stair-Light-Sketches auf den ESP8266.
# Nutzung: ./upload-to-esp8266.sh [PORT]
# Ohne PORT wird /dev/cu.usbserial-0001 verwendet.
# Verfügbare Ports: arduino-cli board list

set -e
cd "$(dirname "$0")"
PORT="${1:-/dev/cu.usbserial-0001}"

echo "→ Compile..."
arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light

echo "→ Upload auf $PORT..."
arduino-cli upload -p "$PORT" --fqbn esp8266:esp8266:nodemcu rgbw_stair_light

echo "→ Fertig."
