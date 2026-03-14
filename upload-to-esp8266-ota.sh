#!/bin/bash
# Upload des Stair-Light-Sketches per OTA (WiFi) auf den ESP8266.
# ESP muss im gleichen WLAN sein und die aktuelle Firmware mit ArduinoOTA laufen.
#
# Nutzung: ./upload-to-esp8266-ota.sh <ESP-IP oder Hostname> [--debug]
# Mit --debug: espota.py wird direkt mit -d -r aufgerufen → detaillierte Fehlerausgabe.
# Beispiele: ./upload-to-esp8266-ota.sh stairlight-testbed.local
#            ./upload-to-esp8266-ota.sh 192.168.1.100 --debug

set -e
cd "$(dirname "$0")"
TARGET="${1:?Ziel fehlt. Nutzung: $0 <IP oder Hostname> [--debug]}"
DEBUG_OTA=""
[[ "${2:-}" == "--debug" ]] && DEBUG_OTA=1

# .local-Hostnamen (mDNS) in IP auflösen – arduino-cli akzeptiert nur IP für OTA
if [[ "$TARGET" == *".local" ]]; then
  echo "→ Resolve $TARGET ..."
  IP=$(ping -c 1 -n "$TARGET" 2>/dev/null | sed -n 's/.*(\([0-9.]*\)).*/\1/p')
  if [[ -z "$IP" ]]; then
    echo "Fehler: $TARGET konnte nicht aufgelöst werden (mDNS/WLAN ok?)." >&2
    exit 1
  fi
  echo "   → $IP"
else
  IP="$TARGET"
fi

echo "→ Compile..."
if [[ -n "$DEBUG_OTA" ]]; then
  BUILD_DIR="./build"
  rm -rf "$BUILD_DIR"
  arduino-cli compile --fqbn esp8266:esp8266:nodemcu --build-path "$BUILD_DIR" rgbw_stair_light
  BIN=$(ls "$BUILD_DIR"/*.bin 2>/dev/null | head -1)
  if [[ -z "$BIN" ]]; then
    echo "Fehler: Keine .bin in $BUILD_DIR gefunden." >&2
    exit 1
  fi
  ESPOTA=$(find "$HOME/Library/Arduino15/packages/esp8266" -name "espota.py" 2>/dev/null | head -1)
  PYTHON=$(find "$HOME/Library/Arduino15/packages/esp8266/tools" -name "python3" -type f 2>/dev/null | head -1)
  if [[ -z "$ESPOTA" || -z "$PYTHON" ]]; then
    echo "Fehler: espota.py oder python3 im ESP8266-Paket nicht gefunden." >&2
    exit 1
  fi
  echo "→ OTA-Upload auf $IP (Debug + Fortschritt)..."
  exec "$PYTHON" -I "$ESPOTA" -i "$IP" -p 8266 --auth= -d -r -f "$BIN"
fi

arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light

echo "→ OTA-Upload auf $IP..."
arduino-cli upload -p "$IP" --fqbn esp8266:esp8266:nodemcu \
  -l network \
  -F password= \
  --discovery-timeout 30s \
  rgbw_stair_light

echo "→ Fertig."
