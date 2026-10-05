#!/bin/bash
# Upload des Stair-Light-Sketches per OTA (WiFi) auf einen oder mehrere ESP8266.
# ESP muss im gleichen WLAN sein und die aktuelle Firmware mit ArduinoOTA laufen.
#
# Nutzung: ./upload-to-esp8266-ota.sh <IP oder Hostname> [<IP oder Hostname> ...] [--debug]
# Kompiliert nur, wenn sich seit dem letzten Build etwas in rgbw_stair_light/ geändert hat;
# mehrere Ziele bekommen nacheinander dieselbe .bin.
# Mit --debug: espota.py mit -d → detaillierte Fehlerausgabe.
# Beispiele: ./upload-to-esp8266-ota.sh stairlight-testbed.local
#            ./upload-to-esp8266-ota.sh 192.168.1.50 192.168.1.51

set -e
cd "$(dirname "$0")"
TARGETS=()
DEBUG_OTA=""
for a in "$@"; do
  if [[ "$a" == "--debug" ]]; then DEBUG_OTA="-d"; else TARGETS+=("$a"); fi
done
if [[ ${#TARGETS[@]} -eq 0 ]]; then
  echo "Ziel fehlt. Nutzung: $0 <IP oder Hostname> [<IP oder Hostname> ...] [--debug]" >&2
  exit 1
fi

BUILD_DIR="./build"
BIN="$BUILD_DIR/rgbw_stair_light.ino.bin"
if [[ ! -f "$BIN" || -n "$(find rgbw_stair_light -type f -newer "$BIN" | head -1)" ]]; then
  echo "→ Compile..."
  arduino-cli compile --fqbn esp8266:esp8266:nodemcu --build-path "$BUILD_DIR" rgbw_stair_light
else
  echo "→ Keine Änderungen seit dem letzten Build – nutze $BIN"
fi

ESPOTA=$(find "$HOME/Library/Arduino15/packages/esp8266" -name "espota.py" 2>/dev/null | head -1)
PYTHON=$(find "$HOME/Library/Arduino15/packages/esp8266/tools" -name "python3" -type f 2>/dev/null | head -1)
if [[ -z "$ESPOTA" || -z "$PYTHON" ]]; then
  echo "Fehler: espota.py oder python3 im ESP8266-Paket nicht gefunden." >&2
  exit 1
fi

FAILED=()
for TARGET in "${TARGETS[@]}"; do
  IP="$TARGET"
  # .local-Hostnamen (mDNS) in IP auflösen – espota braucht eine IP
  if [[ "$TARGET" == *".local" ]]; then
    IP=$(ping -c 1 -n "$TARGET" 2>/dev/null | sed -n 's/.*(\([0-9.]*\)).*/\1/p')
    if [[ -z "$IP" ]]; then
      echo "Fehler: $TARGET konnte nicht aufgelöst werden (mDNS/WLAN ok?)." >&2
      FAILED+=("$TARGET"); continue
    fi
  fi
  echo "→ OTA-Upload auf $TARGET ($IP)..."
  if ! "$PYTHON" -I "$ESPOTA" -i "$IP" -p 8266 --auth= -r $DEBUG_OTA -f "$BIN"; then
    FAILED+=("$TARGET")
  fi
done

if [[ ${#FAILED[@]} -gt 0 ]]; then
  echo "Fehlgeschlagen: ${FAILED[*]}" >&2
  exit 1
fi
echo "→ Fertig: ${TARGETS[*]}"
