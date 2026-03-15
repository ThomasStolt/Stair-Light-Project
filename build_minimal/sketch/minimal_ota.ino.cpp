#line 1 "/Users/thomasstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project/minimal_ota/minimal_ota.ino"
// Minimaler OTA-Test: nur WiFi + ArduinoOTA, keine LEDs/PIR/NTP.
// Wenn OTA mit diesem Sketch funktioniert, liegt das Problem am vollen Stair-Light-Sketch (Speicher/Last).
//
// 1. credentials.h aus rgbw_stair_light/ hierher kopieren (oder Verknüpfung anlegen).
// 2. Per USB flashen: ./upload-minimal-via-usb.sh
// 3. OTA testen: ./upload-minimal-ota.sh stairlight-testbed.local --debug

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "credentials.h"

#line 14 "/Users/thomasstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project/minimal_ota/minimal_ota.ino"
void setup();
#line 49 "/Users/thomasstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project/minimal_ota/minimal_ota.ino"
void loop();
#line 14 "/Users/thomasstolt/Library/CloudStorage/OneDrive-Persönlich/Documents/Github/Stair-Light-Project/minimal_ota/minimal_ota.ino"
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Minimal OTA Test ===");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi ");
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t) < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Kein WiFi. Halt.");
    return;
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() {
    Serial.printf("OTA Start – Heap: %u\n", ESP.getFreeHeap());
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA Ende – Neustart in 2 s");
    delay(2000);
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("OTA Fehler %u\n", e);
  });
  ArduinoOTA.begin();
  Serial.println("OTA bereit. Warte auf Upload...");
}

void loop() {
  ArduinoOTA.handle();
  delay(10);
}

