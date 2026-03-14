// STAIR LIGHT PROJECT - started October 2016
// ==========================================
//
// This project is using NeoPixels to illuminate individual steps of a flight of stairs
// automatically, while someone is walking up (or down) the stairs. The project is based  
// on an ESP8266 as microcontroller, strips of SK2812 as LEDs (should be compatible to
// WS2812) and two SR501 as motion sensors. An ESP8266 is used as the microcontroller.
// I have tried to make it easy to adapt this sketch to your own needs.
// You can e.g. change the number of steps of your stairs (STEPS) as well as
// the 'width' of your stairs, in terms of how many LEDs are you using per step (WIDTH).
// I have written a few animations, much of this code is based on the strandtest code
// example from adafruit, with some adaptations however.
//
// This is pretty much work in progress.
//
// There is some tweaking needed depending on local parameters with the PIR sensors,
// the ANIM_DURATION and the delay after each animation. For me it worked when I set
// the activation time potentiometer to minimum (which results to about 4 seconds of
// the pin set to high), an ANIM_DURATION of 20 seconds (because in my house, one needs
// about 10 seconds to go up the stairs at normal speed) and a delay after each
// animation of 7 seconds. You will have to play around to fit your needs.
//
//
// last update 30.03.2018
//

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include "credentials.h"
#include <Ticker.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

// Pin Assignment: it turns out that GPIO 15 and 2 influence the boot mode
// of the ESP8266, so they should not be used ever.
//

#define NEOPIXEL_PIN  14          // Pin D5 == GPIO 14 -> NeoPixels
#define PIR1 16                   // Pin D0 == GPIO 16 -> PIR Sensor 1
#define PIR2 4                    // Pin D2 == GPIO 4  -> Pir Sensor 2
#define STEPS 16                  // how many steps do the stairs have?
#define WIDTH 27                  // how many LEDs per step do we have?
#define NUM_LEDS (STEPS * WIDTH)  // how many LEDs do we have overall?
#define ANIM_DURATION 20000       // how long is the animation active max? If after this time the second
                                  // IR sensor is not triggered, we call the end of the animation
// if BRIGHNESS is too small (around 10 or less), the animation appears 'skippy', i.e. not smooth
// that is because there are only a few (10) levels of brighness for each color, so this is normal
#define BRIGHTNESS 255            // limit brightness of the strip
#define USE_SERIAL Serial
#define DEBUG 1   // 1 = Serieller Monitor zeigt PIR-Zustand und Trigger

ESP8266WiFiMulti WiFiMulti;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800);



int gammaw[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

#include "parking.h"

WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", ((0*60*60)+(0*60))); // CET = GMT + 1:00

Ticker secondTick;
volatile int watchdogCount = 0;

void ISRwatchdog() {
  watchdogCount++;
  if (watchdogCount == 360) {
    Serial.println();
    Serial.println("the watchdog bites!!!");
    ESP.restart();
  }

}


void setup() {
  // Setting up the serial line
  Serial.begin(115200);
  USE_SERIAL.setDebugOutput(true);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  // LEDs sofort ausschalten (bevor WiFi/OTA – sonst Strip-Zustand unbestimmt)
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  setAll(0, 0, 0, 0);
  strip.show();

  // secondTick.attach(1,ISRwatchdog);


  for( uint8_t t = 4; t > 0; t-- ) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  // ========================================================================================
  // WiFi: SSID und Passwort aus .credentials (steht in .gitignore).
  // ========================================================================================
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  const unsigned long wifiTimeout = 10000;  // 10 s, danach ohne WiFi weitermachen (Testbed)
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart) < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Kein WiFi (Timeout). Fahre ohne Netzwerk fort – PIR/LED funktionieren.");
  }
  // print MAC address, uncomment if needed
  // byte mac[6];
  // WiFi.macAddress(mac);
  // Serial.print("MAC: ");
  // for (int i = 0; i < 5; i++) {
  //   Serial.print(mac[i], HEX);
  //   Serial.print(":");
  // }
  // Serial.println(mac[5], HEX);
  // ========================================================================================
  // ========================================================================================

  
  // OTA: Du pushst neue Firmware von Cursor/PC aus (arduino-cli upload -p <ESP-IP>).
  // ESP holt nichts selbst von einem Server.
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.onStart([]() {
      Serial.printf("OTA Start – freier Heap: %u Byte\n", ESP.getFreeHeap());
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA Ende – warte 2 s vor Neustart (damit OK an PC geht).");
      delay(2000);  // Verhindert Timeout in espota: ESP muss OK senden, bevor er neu startet
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA Fortschritt: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA Fehler[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive");
      else if (error == OTA_END_ERROR) Serial.println("End");
    });
    ArduinoOTA.begin();
#if DEBUG
    Serial.println("OTA bereit. Upload von PC: arduino-cli upload -p " + WiFi.localIP().toString() + " ...");
#endif
  }

  // Strip bereits am Anfang initialisiert; hier nur sicherstellen, dass aus
  setAll(0, 0, 0, 0);
  strip.show();

  // initialise the random generator
  randomSeed(ESP.getCycleCount());
  
  pinMode(PIR1, INPUT);
  pinMode(PIR2, INPUT);

#if DEBUG
  Serial.println();
  Serial.println("=== Setup fertig ===");
  Serial.println("LEDs aus. PIR1 = D0/GPIO16 (oben), PIR2 = D2/GPIO4 (unten).");
  Serial.println("Warte auf Bewegung... (PIR=HIGH = ausgelöst)");
  Serial.println("================================");
#endif
}
  
void loop() {
  Serial.println("");
  int count = 0;
  String dir = "";
  int currenttime = 0;
  int lastPir1 = -1, lastPir2 = -1;  // für Debug: Änderung erkennen
  static int lastAnimation = 0;      // 0 = noch keine, 1–4 = letzte gewählte Animation (keine Doppel)
  while (true) {
    ArduinoOTA.handle();
    watchdogCount = 0;
    int pir1 = digitalRead(PIR1);
    int pir2 = digitalRead(PIR2);

#if DEBUG
    // Alle ~2 s: PIR-Zustand ausgeben (Heartbeat)
    if (count % 20 == 0) {
      Serial.printf("[%lu] PIR1=%s PIR2=%s  (PIR1=UP, PIR2=DOWN)\n",
                    millis() / 1000, pir1 == HIGH ? "HIGH" : "low ", pir2 == HIGH ? "HIGH" : "low ");
    }
    // Bei Zustandsänderung sofort melden
    if (pir1 != lastPir1 || pir2 != lastPir2) {
      Serial.printf("      PIR geändert -> PIR1=%s PIR2=%s\n", pir1 == HIGH ? "HIGH" : "low ", pir2 == HIGH ? "HIGH" : "low ");
      lastPir1 = pir1;
      lastPir2 = pir2;
    }
#endif

    if (pir1 == HIGH) { dir = "UP"; }
    if (pir2 == HIGH) { dir = "DOWN"; }

    if (dir != "") {
#if DEBUG
      Serial.println(">>> PIR ausgelöst: " + dir + " -> starte Animation");
#endif
      if (currenttime > 1537903800 && currenttime < 1537999199) {
        birthday(dir);
      } else {
        // Zufall 1–4, aber nicht dieselbe Animation wie beim letzten Mal
        int choice;
        do {
          choice = random(1, 5);
        } while (choice == lastAnimation);
        lastAnimation = choice;

        switch (choice) {
          case 1:
            simpleFadeToRandom(dir);
            break;
          case 2:
            rainbowSteps(dir);
            break;
          case 3:
            FadeToFullBrightness(dir);
            break;
          case 4:
            starSparkle(dir);
            break;
        }
      }
    }
    dir = "";

    if (count++ > 100) {
      currenttime = ntpClient.getUnixTime();
#if DEBUG
      Serial.printf("NTP Zeit: %d\n", currenttime);
#endif
      count = 0;
    }
    yield();
    delay(100);
  }
}

