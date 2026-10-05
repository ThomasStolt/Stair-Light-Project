#!/bin/bash
# OTA-Upload mit temporär deaktivierter Firewall.
# Firewall wird ausgeschaltet, Upload ausgeführt, danach wieder eingeschaltet –
# auch bei Fehler oder Abbruch (Ctrl+C). Einmalige Passwortabfrage am Anfang.
#
# Nutzung wie upload-to-esp8266-ota.sh, z.B.:
#   ./upload-ota-firewall-ok.sh stairlight-testbed.local
#   ./upload-ota-firewall-ok.sh 192.168.1.50 192.168.1.51
#   ./upload-ota-firewall-ok.sh 192.168.1.50 --debug
#
# Für den Stair-Light-Sketch (nicht Minimal-Test).

set -e
cd "$(dirname "$0")"
SOCKETFILTERFW="/usr/libexec/ApplicationFirewall/socketfilterfw"

echo "Firewall wird kurz für OTA deaktiviert..."
sudo "$SOCKETFILTERFW" --setglobalstate off
trap 'sudo "$SOCKETFILTERFW" --setglobalstate on >/dev/null; echo "Firewall wieder aktiviert."' EXIT

./upload-to-esp8266-ota.sh "$@"
