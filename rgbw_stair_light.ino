// STAIR LIGHT PROJECT - started October 2016
// ==========================================
//
// This project is using NeoPixels to illumiate individual steps of a set of stairs
// automatically, while someone is walking up (or down) the stairs. The SR501 is being used
// as motion sensor. An ESP8266 is used as the microcontroller.
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
// the pin set to high), an ANIM_DURATION of 20 seconds (because in my hous, one needs
// about 10 seconds to go up the stairs at normal speed) and a delay after each
// animation of 7 seconds. You will have to play around to fit your needs.
//
// last update 14.03.2017
//


#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>  // do we need this?
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "parking.h"
#include "credentials.h"

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define NEOPIXEL_PIN 14           // which pin are the LEDs connected to?
#define PIR1_PIN 5                // Pin D0 GPIO 5
#define PIR2_PIN 16               // Pin D0 GPIO 16
#define STEPS 5                   // how many steps do the stairs have?
#define WIDTH 27                  // how many LEDs per step do we have?
#define NUM_LEDS  136             // how many LEDs do we have overall?
#define ANIM_DURATION 20000       // how long is the animation active max? Guessing 20 seconds here
// if BRIGHNESS is too small (around 10 or less), the animation appears 'skippy', i.e. not smooth
// that is because there are only a few (10) levels of brighness for each color, so this is normal
#define BRIGHTNESS 55             // limit brightness of the strip
#define USE_SERIAL Serial
#define UPDATE_SERVER "http://192.168.2.7"

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


void setup() {
  // Setting up the serial line
  Serial.begin(115200);
  // USE_SERIAL.setDebugOutput(true);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for( uint8_t t = 4; t > 0; t-- ) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  // setting up WiFi and printing information
  WiFiMulti.addAP(mySSID, myPASSWORD);
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(mac[i], HEX);
    Serial.print(":");
  }
  Serial.println(mac[5], HEX);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  


  
  strip.setBrightness(BRIGHTNESS);
  strip.begin(); // prepare the data pin for NeoPixel output
  strip.show(); // Initialize all pixels to 'off'
  pinMode(PIR1_PIN, INPUT);
  pinMode(PIR2_PIN, INPUT);
  
}
  
void loop() {

  int count = 0, i;


  // if((WiFiMulti.run() == WL_CONNECTED)) {
  //   t_httpUpdate_return ret = ESPhttpUpdate.update("http://192.168.2.7/rgbw_stair_light.bin");
    // t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin");
    // Serial.println("Jetzt hier 2!");
    // switch(ret) {
      // case HTTP_UPDATE_FAILED:
        // USE_SERIAL.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      // break;
      // case HTTP_UPDATE_NO_UPDATES:
        // USE_SERIAL.println("HTTP_UPDATE_NO_UPDATES");
      // break;
      // case HTTP_UPDATE_OK:
        // USE_SERIAL.println("HTTP_UPDATE_OK");
      // break;
    // }
  // }


  // Rainbow aninmation testing
  i = 0;
  int j = 0;
  int k = 0;
  int color_space;
  while (true) {
    if ( i >= 256) {
      i = 0;
    }
    color_space = (int) 256 / STEPS;
    for ( j = 1; j <= STEPS; j++ ) {
      if ( ( i + color_space * j ) > 255) {
        k = ( i + color_space * j ) - 256;
      }
      else {
        k = ( i + color_space * j );
        // yield();
      }
      setStep(j, Wheel(k));
    }
    i = i + 2;
  }


  
  uint32 s_timer, c_timer; // start time and current time
  int val1, val2; // Value for PIR Sensor 1 and 2
  while (true) {
    val1 = digitalRead(PIR1_PIN);  // read input value of PIR 1
    val2 = digitalRead(PIR2_PIN);  // read input value of PIR 2
    if (val1 == HIGH) {            // check if the input is HIGH
      Serial.println("UP PIR Sensor 1 Motion detected!");
      s_timer = millis();
      for ( i = 1; i <= STEPS; i++ ) {
        fadeInSingleStep(i, 500, 50, 50, 50, 50);
      }
      val2 = digitalRead(PIR2_PIN);
      c_timer = millis();
      // wait until either time elapsed or second PIR triggered
      while ( ! ( val2 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
        yield();
        delay(100);
        val2 = digitalRead(PIR2_PIN);
        if (val2 == HIGH) {
          Serial.println("UP PIR Sensor 2 Motion detected!");
        }
        c_timer = millis();
      }
      // end animation
      for ( i = 1; i <= STEPS ; i++ ) {
        fadeOutSingleStep(i, 500, 50, 50, 50, 50);
      }
      val2 = LOW;
      delay(7000);
    } else if ( val2 == HIGH ) {
      Serial.println("UP PIR Sensor 2 Motion detected!");
      s_timer = millis();
      for ( i = STEPS; i >= 1; i-- ) {
        fadeInSingleStep(i, 500, 50, 50, 50, 50);
      }
      val1 = digitalRead(PIR1_PIN);
      c_timer = millis();
      // wait until either time elapsed or second PIR triggered
      while ( ! ( val1 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
        yield();
        delay(100);
        val1 = digitalRead(PIR1_PIN);
        if (val1 == HIGH) {
          Serial.println("UP PIR Sensor 1 Motion detected!");
        }
        c_timer = millis();
      }
      // end animation
      for ( i = STEPS; i >= 1 ; i-- ) {
        fadeOutSingleStep(i, 500, 50, 50, 50, 50);
      }
      val1 = LOW;
      delay(7000);
    }
    count++;
    Serial.print(".");
    if (count>100) {
      Serial.println(".");
      count = 0;
    }
  yield();
  delay(200);
  }
}

// sets all NeoPixels of step s to the color c

void setStep(int s, int c){
  int step_start = (s - 1) * WIDTH;
  int step_end = step_start + WIDTH;
  for (int i = step_start; i < step_end; i++ ) {
    strip.setPixelColor(i, c);
    yield();
  }
  strip.show();
}


// sets all NeoPixels of step s to a random colour per pixel
void setStepRndm(int s, int c){
  // defines, which pixel is the beginning of the step
  int step_start = (s - 1) * WIDTH;
  // defines, which pixel is the end of the step
  int step_end = step_start + WIDTH;
  for(int i=step_start;i<step_end;i++){
    c = random(16711680);
    // Serial.println(c);
    strip.setPixelColor(i, c);
    yield();
  }
  strip.show();
}



void fadeStep(int red, int green, int blue, int white){
  int i, j, s;
  // defines, which pixel is the beginning of the step
  for (s=1;s<=STEPS;s++) {
    int step_start = (s - 1) * WIDTH;
    // defines, which pixel is the end of the step
    int step_end = step_start + WIDTH;
    Serial.print("Step start: ");
    Serial.println(step_start+1);
    Serial.print("Step end: ");
    Serial.println(step_end);
    for (i=0;i<100;i++) {
      for (j=step_start;j<step_end;j++) {
        strip.setPixelColor(j, gammaw[i], 0, gammaw[i], gammaw[i]);
        yield();
      }
      strip.show();
    }
  }
}

// This function fadeInSingleStep will fade a single step number step_number from zero (off)
// to the values red, green, blue within the allotted time fade_time_ms (in milliseconds)
// A reverse functionality is provided by the function fadeOutSingleStep

void fadeInSingleStep(int step_number, int fade_time_ms, int red, int green, int blue, int white){
  int i, j;
  float i_1, step_width;
  uint32_t t_1, t_2;
  uint32_t cpu_time_used;
  // figure out, which pixel is the beginning of the step
  int step_start = (step_number - 1) * WIDTH;
  // figure out, which pixel is the end of the step
  int step_end = step_start + WIDTH;
  // figure out the factor for red, green, blue and white
  float factor_r = red / 256;
  float factor_g = green / 256;
  float factor_b = blue / 256;
  float factor_w = white / 256;
  step_width = (float) fade_time_ms / 1488;
  i_1 = 0;
  for ( i = 0; i < 256; i = (int) i_1 ) {
    i_1 = i_1 + 1 / step_width;
    for (j=step_start;j<step_end;j++) {
      strip.setPixelColor(j, 0, 0, 0, gammaw[i]);
      yield();
    }
    // Serial.println(gammaw[i]);
    strip.show();
    // delay(10);
  }
  // Serial.println("Animantion done!");
  // delay(1000);
}

void fadeOutSingleStep(int step_number, int fade_time_ms, int red, int green, int blue, int white){
  int i, j;
  float i_1, step_width;
  uint32_t t_1, t_2;
  uint32_t cpu_time_used;
  // figure out, which pixel is the beginning of the step
  int step_start = (step_number - 1) * WIDTH;
  // figure out, which pixel is the end of the step
  int step_end = step_start + WIDTH;
  // figure out the factor for red, green, blue and white
  float factor_r = red / 256;
  float factor_g = green / 256;
  float factor_b = blue / 256;
  float factor_w = white / 256;
  step_width = (float) fade_time_ms / 1488;
  i_1 = 256;
  for ( i = 256; i > 0; i = (int) i_1 ) {
    i_1 = i_1 - 1 / step_width;
    for (j=step_start;j<step_end;j++) {
      strip.setPixelColor(j, 0, 0, 0, gammaw[i]);
      yield();
    }
    // Serial.println(gammaw[i]);
    strip.show();
    // delay(10);
  }
  // Serial.println("Animantion done!");
  // delay(1000);
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
    strip.setPixelColor(i, strip.Color(red,green,blue,white)); // Moderately bright green color.
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
