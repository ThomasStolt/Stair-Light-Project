// STAIR LIGHT PROJECT - started October 2016
// ==========================================
//
// This project is using NeoPixels to illuminate individual steps of a flight of stairs
// automatically, while someone is walking up (or down) the stairs. The project is based
// on an ESP8266 as microcontroller, strips of SK2812 as LEDs (should be compatible to
// WS2812) and two SR501 as motion
// sensor. An ESP8266 is used as the microcontroller.
// I have written this sketch so that you should be easily able to adapt it to your
// own needs. You can e.g. adapt the number of steps of your stairs (STEPS) as well as
// the 'width' of your stairs, in terms of how many LEDs are you using per step (WIDTH).
// I have written a few animations, most of this code is based on the strandtest code
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
#include <WiFiClient.h>  // do we need this?
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <credentials.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

// Pin Assignment: it turns out that GPIO 15 and 2 influence the boot mode
// of the ESP8266, should not be used
//

#define NEOPIXEL_PIN  14          // Pin D5 == GPIO 14 -> NeoPixels
#define PIR1_PIN 16               // Pin D0 == GPIO 16 -> PIR Sensor 1
#define PIR2_PIN 4                // Pin D2 == GPIO 4  -> Pir Sensor 2
// #define STEPS 16                    // how many steps do the stairs have?
#define STEPS 5                    // how many steps do the stairs have?
#define WIDTH 27                  // how many LEDs per step do we have?
// #define NUM_LEDS  432             // how many LEDs do we have overall?
#define NUM_LEDS  135             // how many LEDs do we have overall?
#define ANIM_DURATION 20000       // how long is the animation active max? Guessing 20 seconds here
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

void setup() {
  // Setting up the serial line
  Serial.begin(115200);
  USE_SERIAL.setDebugOutput(true);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for( uint8_t t = 4; t > 0; t-- ) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  // setting up WiFi and printing information
  // this comes from .arduino15/packages/esp8266/hardware/esp82766/2.4.1/libaries/credentials
  // and needs to be created. Only here to hide my own credentials from Github ;)
  WiFi.begin(mySSID, myPass);
  
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(mac[i], HEX);
    Serial.print(":");
  }
  Serial.println(mac[5], HEX);
  
  // Getting updates OTA
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
 
  strip.setBrightness(BRIGHTNESS);
  strip.begin(); // prepare the data pin for NeoPixel output
  strip.show(); // Initialize all pixels to 'off'
  
  pinMode(PIR1_PIN, INPUT);
  pinMode(PIR2_PIN, INPUT);
  
}
  
void loop() {
  Serial.println("");
  int i, count = 0;
  uint32 s_timer, c_timer; // start time and current time
  int val1, val2; // Value for PIR Sensor 1 and 2
  setAll(0,0,0,0);
  while (true) {
    val1 = digitalRead(PIR1_PIN);
    yield();
    delay(50);
    val2 = digitalRead(PIR2_PIN);
    yield();
    delay(50);
    if ( val1 == HIGH ) {
      simple_fade("UP");
      val1 = digitalRead(PIR1_PIN);
    }
    if ( val2 == HIGH ) {
      simple_fade("DOWN");
      val2 = digitalRead(PIR2_PIN);  // read input value of PIR 2
    }
    Serial.print(".");
    if ( count++ > 100 ) {
      Serial.println("");
      count = 0;
    }
    yield();
    delay(200);
  }
}









// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// Fade Function:
void FadeInOut(byte red, byte green, byte blue, byte white){
  float r, g, b, w;
  for(int k = 0; k < 156; k=k+1) { 
    r = (k/156.0)*red;
    g = (k/156.0)*green;
    b = (k/156.0)*blue;
    w = (k/156.0)*white;
    setAll(r,g,b,w);
    strip.show();
  }
  
  for(int k = 156; k >= 0; k=k-2) {
    r = (k/156.0)*red;
    g = (k/156.0)*green;
    b = (k/156.0)*blue;
    w = (k/156.0)*white;
    setAll(r,g,b,w);
    strip.show();
  }
}

void setAll(int red, int green, int blue, int white){

  for(int i=0;i<NUM_LEDS;i++){
    // pixels.Color takes RGBW values, from 0,0,0,0 up to 255,255,255,255
    strip.setPixelColor(i, strip.Color(red,green,blue,white));
    // strip.show(); // This sends the updated pixel color to the hardware.
  }
}

void pulseWhite(uint8_t wait) {
  for(int j = 0; j < 256 ; j++){
      for(uint16_t i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0,0,0, gammaw[j] ) );
        }
        delay(wait);
        strip.show();
      }

  for(int j = 255; j >= 0 ; j--){
      for(uint16_t i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0,0,0, gammaw[j] ) );
        }
        delay(wait);
        strip.show();
      }
}


void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops) {
  float fadeMax = 100.0;
  int fadeVal = 0;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for(int k = 0 ; k < rainbowLoops ; k ++){
    
    for(int j=0; j<256; j++) { // 5 cycles of all colors on wheel

      for(int i=0; i< strip.numPixels(); i++) {

        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);

        redVal = red(wheelVal) * float(fadeVal/fadeMax);
        greenVal = green(wheelVal) * float(fadeVal/fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal/fadeMax);

        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );

      }

      // First loop, fade in!
      if(k == 0 && fadeVal < fadeMax-1) {
          fadeVal++;
      }

      // Last loop, fade out!
      else if(k == rainbowLoops - 1 && j > 255 - fadeMax ){
          fadeVal--;
      }

        strip.show();
        delay(wait);
    }
  
  }



  delay(500);


  for(int k = 0 ; k < whiteLoops ; k ++){

    for(int j = 0; j < 256 ; j++){

        for(uint16_t i=0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, strip.Color(0,0,0, gammaw[j] ) );
          }
          strip.show();
        }

        delay(2000);
    for(int j = 255; j >= 0 ; j--){

        for(uint16_t i=0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, strip.Color(0,0,0, gammaw[j] ) );
          }
          strip.show();
        }
  }

  delay(500);


}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength ) {
  
  if(whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;

  int head = whiteLength - 1;
  int tail = 0;

  int loops = 3;
  int loopNum = 0;

  static unsigned long lastTime = 0;

  while(true){
    for(int j=0; j<256; j++) {
      for(uint16_t i=0; i<strip.numPixels(); i++) {
        if((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) ){
          strip.setPixelColor(i, strip.Color(0,0,0, 255 ) );
        }
        else{
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }
        
      }

      if(millis() - lastTime > whiteSpeed) {
        head++;
        tail++;
        if(head == strip.numPixels()){
          loopNum++;
        }
        lastTime = millis();
      }

      if(loopNum == loops) return;
    
      head%=strip.numPixels();
      tail%=strip.numPixels();
        strip.show();
        delay(wait);
    }
  }  

}

// Full White Cold
void fullWhiteC() {
  
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0,0,0, BRIGHTNESS ) );
    }
      strip.show();
}

// Full White Warm
void fullWhiteW() {
  
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(BRIGHTNESS,BRIGHTNESS,BRIGHTNESS, 0 ) );
    }
      strip.show();
}

// Full White Warm and Cold
void fullWhiteWC() {
  
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(BRIGHTNESS,BRIGHTNESS,BRIGHTNESS, BRIGHTNESS ) );
    }
      strip.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256 * 5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

uint8_t red(uint32_t c) {
  return (c >> 8);
}
uint8_t green(uint32_t c) {
  return (c >> 16);
}
uint8_t blue(uint32_t c) {
  return (c);
}
