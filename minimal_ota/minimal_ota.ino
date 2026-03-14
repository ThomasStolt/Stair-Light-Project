// Minimal OTA test: WiFi + ArduinoOTA only, no LEDs/PIR/NTP.
// If OTA works with this sketch, the issue is with the full Stair Light sketch (memory/load).
//
// 1. Copy credentials.h from rgbw_stair_light/ here (or create a link).
// 2. Flash via USB: ./upload-minimal-via-usb.sh
// 3. Test OTA: ./upload-minimal-ota.sh stairlight-testbed.local --debug

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "credentials.h"

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
    Serial.println("No WiFi. Stopping.");
    return;
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() {
    Serial.printf("OTA start – heap: %u\n", ESP.getFreeHeap());
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA end – restart in 2 s");
    delay(2000);
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("OTA error %u\n", e);
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready. Waiting for upload...");
}

void loop() {
  ArduinoOTA.handle();
  delay(10);
}
