#!/bin/bash
# OTA-Upload des Minimal-Sketches (zum Testen, ob OTA grundsätzlich funktioniert).
# Vorher muss der Minimal-Sketch per USB laufen: ./upload-minimal-via-usb.sh
#
# Nutzung: ./upload-minimal-ota.sh <ESP-IP oder Hostname> [--debug]

set -e
cd "$(dirname "$0")"
TARGET="${1:?Ziel fehlt. Nutzung: $0 <IP oder Hostname> [--debug]}"
DEBUG_OTA=""
[[ "${2:-}" == "--debug" ]] && DEBUG_OTA=1

if [[ "$TARGET" == *".local" ]]; then
  echo "→ Resolve $TARGET ..."
  IP=$(ping -c 1 -n "$TARGET" 2>/dev/null | sed -n 's/.*(\([0-9.]*\)).*/\1/p')
  if [[ -z "$IP" ]]; then
    echo "Fehler: $TARGET konnte nicht aufgelöst werden." >&2
    exit 1
  fi
  echo "   → $IP"
else
  IP="$TARGET"
fi

echo "→ Compile minimal_ota..."
if [[ -n "$DEBUG_OTA" ]]; then
  BUILD_DIR="./build_minimal"
  rm -rf "$BUILD_DIR"
  arduino-cli compile --fqbn esp8266:esp8266:nodemcu --build-path "$BUILD_DIR" minimal_ota
  BIN=$(ls "$BUILD_DIR"/*.bin 2>/dev/null | head -1)
  if [[ -z "$BIN" ]]; then
    echo "Fehler: Keine .bin in $BUILD_DIR gefunden." >&2
    exit 1
  fi
  ESPOTA=$(find "$HOME/Library/Arduino15/packages/esp8266" -name "espota.py" 2>/dev/null | head -1)
  PYTHON=$(find "$HOME/Library/Arduino15/packages/esp8266/tools" -name "python3" -type f 2>/dev/null | head -1)
  if [[ -z "$ESPOTA" || -z "$PYTHON" ]]; then
    echo "Fehler: espota.py oder python3 nicht gefunden." >&2
    exit 1
  fi
  echo "→ OTA-Upload Minimal-Sketch auf $IP (Debug)..."
  exec "$PYTHON" -I "$ESPOTA" -i "$IP" -p 8266 --auth= -d -r -f "$BIN"
fi

arduino-cli compile --fqbn esp8266:esp8266:nodemcu minimal_ota

echo "→ OTA-Upload auf $IP..."
arduino-cli upload -p "$IP" --fqbn esp8266:esp8266:nodemcu \
  -l network \
  -F password= \
  --discovery-timeout 30s \
  minimal_ota

echo "→ Fertig."
