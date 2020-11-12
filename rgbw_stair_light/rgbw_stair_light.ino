// STAIR LIGHT PROJECT - started October 2016
// ==========================================
//
// This project is using NeoPixels to illuminate individual steps of a flight of stairs
// automatically, while someone is walking up (or down) the stairs. The project is based  
// on an ESP8266 as microcontroller, strips of SK2812 as LEDs (should be compatible to
// WS2812) and two SR501 as motion sensors. An ESP8266 is used as the microcontroller.
// I have tried to make it easy to adapt this sketch to your own needs.
// You can e.g. change the number of steps of your stairs (STEPS) as well as
// the 'width' of your stairs, in terms of how many LEDs you are using per step (WIDTH).
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
//#include <WiFiClient.h> do we need this?  No we don’t!
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
// #include <credentials.h>
#include <Ticker.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

// Pin Assignment: it turns out that GPIO 15 and 2 influence the boot mode
// of the ESP8266, so they should not be used, ever.
//

#define NEOPIXEL_PIN  14          // Pin D5 == GPIO 14 -> NeoPixels
#define PIR1 16                   // Pin D0 == GPIO 16 -> PIR Sensor 1
#define PIR2 4                    // Pin D2 == GPIO 4  -> Pir Sensor 2
#define STEPS 16                  // how many steps do the stairs have?
#define WIDTH 27                  // how many LEDs per step do we have?
#define NUM_LEDS (STEPS * WIDTH)  // how many LEDs do we have overall?
#define ANIM_DURATION 20000       // how long is the animation active max? If after this time the second IR sensor is not triggered, we call the end of the animation
// if BRIGHNESS is too small (around 10 or less), the animation appears 'skippy', i.e. not smooth
// that is because there are only a few (10) levels of brighness for each color, so this is normal
#define BRIGHTNESS 255            // limit brightness of the strip
#define USE_SERIAL Serial
#define OTA_SERVER "http://192.168.2.7"



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
    Serial.println("restarting now");
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

  // secondTick.attach(1,ISRwatchdog);


  for( uint8_t t = 4; t > 0; t-- ) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  // ========================================================================================
  // Setting up WiFi. I am using mySSID and myPass here. On my system, they are defined in a
  // header file named credentials.h (see include statemend above), which I have located at:
  // ~/.arduino15/packages/esp8266/hardware/esp8266/2.4.1/libaries/credentials/
  // It contains only two lines:
  // char mySSID[] = "ssid";
  // char myPass[] = "pass";
  // This is only here to hide my own credentials from Github. You can also just put your own
  // credentials directly into the WifFi.begin("YourWiFi","YourWiFiPass") function.
  // ========================================================================================
  // WiFi.begin(mySSID, myPass);
  WiFi.begin("", "");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".:");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
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

  
  // =======================================================================================
  // Don't even touch this, it took me weeks to get this working. I am not sure why, but it
  // does work now.
  // Getting updates OTA
  // I am thinking of making this a function and calling it at first boot (e.g. if the reset
  // button is pressed) or through an MQTT message from a broker. But it is not that urgent.
  // =======================================================================================
  if((WiFi.status() == WL_CONNECTED)) {
    // t_httpUpdate_return ret = ESPhttpUpdate.update("http://192.168.2.7/iotappstoryv20.php");
    t_httpUpdate_return ret = ESPhttpUpdate.update("http://192.168.2.7/bin/rgbw_stair_light");
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          USE_SERIAL.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
        case HTTP_UPDATE_NO_UPDATES:
          USE_SERIAL.println("HTTP_UPDATE_NO_UPDATES");
        break;
        case HTTP_UPDATE_OK:
          USE_SERIAL.println("HTTP_UPDATE_OK");
        break;
      }
    }
  // =======================================================================================
  // =======================================================================================
  // =======================================================================================
  
  strip.setBrightness(BRIGHTNESS);
  strip.begin(); // prepare the data pin for NeoPixel output
  Serial.println("All pixels should now be off");
  setAll(0,0,0,0);
  strip.show(); // Initialize all pixels to 'off'

  // initialise the random generator
  randomSeed(ESP.getCycleCount());
  
  pinMode(PIR1, INPUT);
  pinMode(PIR2, INPUT);
  
}
  
void loop() {
  Serial.println("");
  int count = 0;   // some nicer debug output, to tell if it's still working
  String dir = ""; // to tell, which direction someone is walking the stairs
  int currenttime;
  while (true) {
    watchdogCount = 0;
    // figure out first, which IR sensor has been triggered
    if ( digitalRead(PIR1) == HIGH ) { dir = "UP"; }
    if ( digitalRead(PIR2) == HIGH ) { dir = "DOWN"; }
    // if one of them has been triggered, choose a random animation function to go to
    if ( dir != "" ) {
      // J's Birthday (My son)
      if ( currenttime > 1531778400 && currenttime < 1531864799 ) {
      // M's Birthday (My daughter)
      if ( currenttime > 1537903800 && currenttime < 1537999199 ) {
        birthday(dir);
      } else {
        switch (random(1,5)) {
  // for testing purposes
        // switch (5) { 
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
    Serial.print(".");
    // Serial.println(currenttime);
    if ( count++ > 100 ) {
      Serial.println("");
      
      currenttime = ntpClient.getUnixTime();
      
      Serial.println(currenttime);
      count = 0;
    }
    yield();
    delay(100);
  }
}



iuh

