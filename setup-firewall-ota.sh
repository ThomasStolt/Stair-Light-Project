#!/bin/bash
# Firewall-Regel für OTA-Upload per CLI setzen.
# Erlaubt dem Arduino-Python (espota), eingehende TCP-Verbindungen vom ESP zu akzeptieren.
# Einmal ausführen mit: sudo ./setup-firewall-ota.sh

set -e
SOCKETFILTERFW="/usr/libexec/ApplicationFirewall/socketfilterfw"

# Python aus dem ESP8266-Arduino-Paket (beliebige Version)
PYTHON_DIR="$HOME/Library/Arduino15/packages/esp8266/tools/python3"
if [[ ! -d "$PYTHON_DIR" ]]; then
  echo "Nicht gefunden: $PYTHON_DIR" >&2
  exit 1
fi
# Neueste Version nehmen (z.B. 3.7.2-post1)
LATEST=$(ls -1 "$PYTHON_DIR" 2>/dev/null | grep -E '^[0-9]' | sort -V | tail -1)
PYTHON_BIN="$PYTHON_DIR/$LATEST/python3"
if [[ ! -x "$PYTHON_BIN" ]]; then
  echo "Nicht gefunden: $PYTHON_BIN" >&2
  exit 1
fi

echo "Python für Firewall: $PYTHON_BIN"
echo ""

# Firewall-Regel setzen (benötigt sudo)
echo "Füge Anwendungen hinzu und erlaube eingehende Verbindungen..."
sudo "$SOCKETFILTERFW" --add "$PYTHON_BIN"
sudo "$SOCKETFILTERFW" --unblockapp "$PYTHON_BIN"

# Oft wird die eingehende Verbindung dem Terminal zugerechnet – deshalb auch erlauben
for app in "/System/Applications/Utilities/Terminal.app" "/Applications/Cursor.app"; do
  if [[ -d "$app" ]]; then
    echo "  → $app"
    sudo "$SOCKETFILTERFW" --add "$app" 2>/dev/null || true
    sudo "$SOCKETFILTERFW" --unblockapp "$app" 2>/dev/null || true
  fi
done

echo ""
echo "Fertig. Bitte OTA erneut testen."
echo "Falls es weiter fehlschlägt: Systemeinstellungen → Netzwerk → Firewall → Optionen:"
echo "  • „Unsichtbarer Modus“ deaktivieren"
echo "  • Prüfen, dass nicht „Alle eingehenden Verbindungen blockieren“ aktiv ist"
echo ""
echo "Prüfen: sudo $SOCKETFILTERFW --listapps | grep -E 'python|Terminal|Cursor'"
