#!/bin/bash
# OTA-Upload mit temporär deaktivierter Firewall.
# Firewall wird ausgeschaltet, Upload ausgeführt, danach wieder eingeschaltet.
# Einmalige Passwortabfrage am Anfang.
#
# Nutzung wie upload-to-esp8266-ota.sh, z.B.:
#   ./upload-ota-firewall-ok.sh stairlight-testbed.local
#   ./upload-ota-firewall-ok.sh stairlight-testbed.local --debug
#   ./upload-ota-firewall-ok.sh 192.168.2.185
#
# Für den Stair-Light-Sketch (nicht Minimal-Test).

set -e
cd "$(dirname "$0")"
SOCKETFILTERFW="/usr/libexec/ApplicationFirewall/socketfilterfw"

# Firewall-Status merken und ausschalten
echo "Firewall wird kurz für OTA deaktiviert..."
sudo "$SOCKETFILTERFW" --setglobalstate off

# OTA ausführen (Fehler durchreichen)
if ! ./upload-to-esp8266-ota.sh "$@"; then
  rc=$?
  sudo "$SOCKETFILTERFW" --setglobalstate on
  echo "Firewall wieder aktiviert."
  exit $rc
fi

# Firewall wieder an
sudo "$SOCKETFILTERFW" --setglobalstate on
echo "Firewall wieder aktiviert."
