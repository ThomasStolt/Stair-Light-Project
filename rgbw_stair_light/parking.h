#include <ArduinoOTA.h>

extern uint32_t g_animDurationOverrideMs;  // if >0: duration for animation (e.g. 10000 = 10 s "Go")

// ===================================================================================
// FUNCTION NAME
// testPIRs
// -----------------------------------------------------------------------------------
// Continuously reads the PIRs and switches the first or last LED of the first step
// to full green
// -----------------------------------------------------------------------------------
void testPIRs () {
  while (true) {
    int val1, val2;
    val1 = digitalRead(PIR1);  // read input value of PIR 1
    val2 = digitalRead(PIR2);  // read input value of PIR 2
    if ( val1 == HIGH ) {
      strip.setPixelColor(0, 0, 200, 0, 50);
    } else if ( val1 == LOW ) {
      strip.setPixelColor(0, 0, 0, 0, 0);
    }
    if ( val2 == HIGH ) {
      strip.setPixelColor(26, 0, 200, 0, 50);
    } else if ( val2 == LOW ) {
      strip.setPixelColor(26, 0, 0, 0, 0);
    }
    strip.show();
    yield();
    delay(20);
  }
}


// ===================================================================================
// NAME
// setAll
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// This function sets all LEDs of strip to the colours passed to it
// -----------------------------------------------------------------------------------
void setAll(int red, int green, int blue, int white){
  for(int i=0;i<NUM_LEDS;i++){
    strip.setPixelColor(i, strip.Color(red,green,blue,white));
  }
}

// ===================================================================================
// NAME
// red, green, blue
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// These functions will take a uint32_t input variable (4 bytes), which contain in the
// first byte the red value, the second byte the green value and in the third byte the
// blue value. The return value is of type uint8_t (i.e. one byte) and contains the
// value of the corresponding colour. This is later needed by some other functions.
// -----------------------------------------------------------------------------------
uint8_t red(uint32_t c) {
  return (c >> 8);
}
uint8_t green(uint32_t c) {
  return (c >> 16);
}
uint8_t blue(uint32_t c) {
  return (c);
}

// ===================================================================================
// NAME
// Wheel
// -----------------------------------------------------------------------------------
// This function takes as input a value from 0 to 255 and returns a uint32_t with a 
// color value for a neopixel. Just like a colour wheel. When counted upwards, the 
// colours are a transitioned from red to green to blue and back to red.

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
// -----------------------------------------------------------------------------------


// ===================================================================================
// NAME
// fadeInSingleStep
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// this fades in all LEDs of a given step from 0 within a given fade time
// -----------------------------------------------------------------------------------
void fadeInSingleStep(int step_number, int fade_time_ms, int red, int green, int blue, int white){
  int i, j; 
  float i_1, step_width;
  uint32_t t_1, t_2;
  // figure out, which pixel is the beginning of the step
  int step_start = (step_number - 1) * WIDTH;
  // figure out, which pixel is the end of the step
  int step_end = step_start + WIDTH;
  // figure out the factor for red, green, blue and white
  float factor_r = float(red) / 256;
  float factor_g = float(green) / 256;
  float factor_b = float(blue) / 256;
  float factor_w = float(white) / 256;
  step_width = (float) fade_time_ms / 1488;
  i_1 = 0;
  for ( i = 0; i < 256; i = (int) i_1 ) {
    i_1 = i_1 + 1 / step_width;
    for (j=step_start;j<step_end;j++) {
      strip.setPixelColor(j, gammaw[int(i*factor_r)],gammaw[int(i*factor_g)],gammaw[int(i*factor_b)], gammaw[int(i*factor_w)]);
      // strip.setPixelColor(j, i, i, i, i);
      yield();
    }
    strip.show();
    handleNetwork();
    yield();
  }
  delay(10);
}
// -----------------------------------------------------------------------------------


// ===================================================================================
// NAME
// fadeOutSingleStep
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// this fades out all LEDs of a given step to 0 within a given fade time
// -----------------------------------------------------------------------------------
void fadeOutSingleStep(int step_number, int fade_time_ms, int red, int green, int blue, int white){
  int i, j;
  float i_1, step_width;
  uint32_t t_1, t_2;
  // figure out, which pixel is the beginning of the step
  int step_start = (step_number - 1) * WIDTH;
  // figure out, which pixel is the end of the step
  int step_end = step_start + WIDTH;
  // figure out the factor for red, green, blue and white
  float factor_r = float(red) / 256;
  float factor_g = float(green) / 256;
  float factor_b = float(blue) / 256;
  float factor_w = float(white) / 256;
  step_width = (float) fade_time_ms / 1488;
  i_1 = 255;
  for ( i = 255; i > 0; i = (int) i_1 ) {
    i_1 = i_1 - 1 / step_width;
    for (j=step_start;j<step_end;j++) {
      strip.setPixelColor(j, gammaw[int(i*factor_r)],gammaw[int(i*factor_g)],gammaw[int(i*factor_b)], gammaw[int(i*factor_w)]);
      yield();
    }
    strip.show();
    handleNetwork();
    yield();
  }
  delay(10);
}
// -----------------------------------------------------------------------------------


// ===================================================================================
// NAME
// FadeToFullBrightness
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// This function fades all LEDs to full brighness
// -----------------------------------------------------------------------------------
void FadeToFullBrightness(String dir){
  Serial.println("FadeFullBrightness");
  int i;
  unsigned long s_timer = millis();
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  if (dir == "UP") {
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) {
      fadeInSingleStep(i, 100, 255, 255, 255, 255);
    }
    while ( millis() - s_timer < limit ) {
      handleNetwork();
      delay(100);
      yield();
    }
    for ( i = 1; i <= STEPS ; i++ ) {
      fadeOutSingleStep(i, 100, 255, 255, 255, 255);
    }
  } else if ( dir == "DOWN" ) {
    Serial.println("Moving down the stairs");
    s_timer = millis();
    for ( i = STEPS; i >= 1; i-- ) {
      fadeInSingleStep(i, 100, 255, 255, 255, 255);
    }
    while ( millis() - s_timer < limit ) {
      handleNetwork();
      delay(100);
      yield();
    }
    for ( i = STEPS; i >= 1 ; i-- ) {
      fadeOutSingleStep(i, 100, 255, 255, 255, 255);
    }
  }
  yield();
}
// -----------------------------------------------------------------------------------


// ===================================================================================
// NAME
// starSparkle
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Sparkling stars on dark blue backdrop
// -----------------------------------------------------------------------------------
void starSparkle(String dir){
  Serial.println("starSparkle");
  int i, minStars = 10, maxStars = 20;
  unsigned long s_timer = millis();
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  int blue = 120;
  int blue_gamma = gammaw[blue];
  if (dir == "UP") {
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) { fadeInSingleStep(i, 75, 0, 0, blue, 0); }
    while ( millis() - s_timer < limit ) {
      handleNetwork();
      for ( i = 1; i < random(minStars,maxStars); i++) {
        strip.setPixelColor(random(0,NUM_LEDS), 255, 255, 255, 255);
      }
      strip.show();
      setAll(0, 0, blue_gamma,0);
      strip.show();
      yield();
    }
    for ( i = 1; i <= STEPS ; i++ ) { fadeOutSingleStep(i, 75, 0, 0, blue, 0); }
  } else if ( dir == "DOWN" ) {
    Serial.println("Moving down the stairs");
    for ( i = STEPS; i >= 1; i-- ) { fadeInSingleStep(i, 75, 0, 0, blue, 0); }
    while ( millis() - s_timer < limit ) {
      handleNetwork();
      for ( i = 1; i < random(minStars,maxStars); i++) {
        strip.setPixelColor(random(0, NUM_LEDS), 255, 255, 255, 255);
      }
      strip.show();
      setAll(0, 0, blue_gamma, 0);
      strip.show();
      yield();
    }
    for ( i = STEPS; i >= 1 ; i-- ) { fadeOutSingleStep(i, 75, 0, 0, blue, 0); }
  }
  yield();
}
// -----------------------------------------------------------------------------------



// ===================================================================================
// NAME
// simpleFadeToRandom
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Fades all stairs to a random colour, no animation
// -----------------------------------------------------------------------------------
void simpleFadeToRandom(String dir){
  Serial.println("SimpleFadeToRandom");
  int i;
  unsigned long s_timer = millis();
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  int red = random(256);
  int green = random(256);
  int blue = random(256);
  int white = random(256);
  if (dir == "UP") {
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) {
      fadeInSingleStep(i, 100, red, green, blue, white);
    }
    while ( millis() - s_timer < limit ) {
      handleNetwork();
      delay(100);
      yield();
    }
    for ( i = 1; i <= STEPS ; i++ ) {
      fadeOutSingleStep(i, 100, red, green, blue, white);
    }
  } else if ( dir == "DOWN" ) {
    Serial.println("Moving down the stairs");
    s_timer = millis();
    for ( i = STEPS; i >= 1; i-- ) {
      fadeInSingleStep(i, 100, red, green, blue, white);
    }
    while ( millis() - s_timer < limit ) {
      handleNetwork();
      delay(100);
      yield();
    }
    for ( i = STEPS; i >= 1 ; i-- ) {
      fadeOutSingleStep(i, 100, red, green, blue, white);
    }
  }
  yield();
}


// ===================================================================================
// NAME
// setStep
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// sets all NeoPixels of step s to the color c
// -----------------------------------------------------------------------------------
void setStep(int s, int c){
  int step_start = (s - 1) * WIDTH;
  int step_end = step_start + WIDTH;
  for (int i = step_start; i < step_end; i++ ) {
    strip.setPixelColor(i, c);
    yield();
  }
  // strip.show();
}


// ===================================================================================
// NAME
// rainbowSteps
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// fades all steps in, to rainbow colours, then rainbow animation, then fade out
// -----------------------------------------------------------------------------------
void rainbowSteps(String dir){
  Serial.println("rainbowSteps");
  int j, k;
  unsigned long s_timer;
  bool timed_out;
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;

  // Fade a single step in/out using gamma-corrected brightness scaled over its exact
  // Wheel colour, so the brightness at fade-end matches the animation start exactly.
  const int FIN  = 12;  // frames per step fade-in  (~160 ms/step)
  const int FOUT =  8;  // frames per step fade-out (~100 ms/step)

  auto fadeStep = [&](int step, bool fadeIn) {
    uint32_t wc = Wheel(byte((step-1)*255/STEPS));
    uint8_t wr = red(wc), wg = green(wc), wb = blue(wc);
    int sp = (step-1)*WIDTH;
    int frames = fadeIn ? FIN : FOUT;
    for (int f = 0; f <= frames; f++) {
      int fi = fadeIn ? f : (frames - f);
      uint8_t brt = gammaw[fi*255/frames];
      uint8_t r = (uint8_t)((uint32_t)wr*brt/255);
      uint8_t g = (uint8_t)((uint32_t)wg*brt/255);
      uint8_t b = (uint8_t)((uint32_t)wb*brt/255);
      for (int p = sp; p < sp+WIDTH; p++)
        strip.setPixelColor(p, strip.Color(r, g, b, 0));
      strip.show();
      handleNetwork();
      yield();
    }
  };

  if (dir == "DOWN") {
    Serial.println("Moving down the stairs");
    for (j = STEPS; j >= 1; j--) fadeStep(j, true);
    s_timer = millis(); timed_out = false;
    for (k = 0; k < 2; k++) {
      for (int i = 0; i < 256; i += 2) {
        if (millis() - s_timer > limit) { timed_out = true; break; }
        handleNetwork();
        for (j = 1; j <= STEPS; j++) setStep(j, Wheel(byte(i + (j-1)*255/STEPS)));
        strip.show();
        yield();
      }
      if (timed_out) break;
    }
    for (j = STEPS; j >= 1; j--) fadeStep(j, false);
  } else if (dir == "UP") {
    Serial.println("Moving up the stairs");
    for (j = 1; j <= STEPS; j++) fadeStep(j, true);
    s_timer = millis(); timed_out = false;
    for (k = 0; k < 2; k++) {
      for (int i = 256; i > 0; i--) {
        if (millis() - s_timer > limit) { timed_out = true; break; }
        handleNetwork();
        for (j = 1; j <= STEPS; j++) setStep(j, Wheel(byte(i + (j-1)*255/STEPS)));
        strip.show();
        yield();
      }
      if (timed_out) break;
    }
    for (j = 1; j <= STEPS; j++) fadeStep(j, false);
  }
}

// ===================================================================================
// NAME
// setStepRndm
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Sets all pixels of a given step to a random colour - ONLY ON BIRTHDAYS!!!
// -----------------------------------------------------------------------------------
void setStepRndm(int s, int c){
  int step_start = (s - 1) * WIDTH;
  int step_end = step_start + WIDTH;
  for(int i=step_start;i<step_end;i++){
    uint32_t col = Wheel((byte)random(255));
    uint8_t rv = (uint8_t)red(col) / 2;    // 50% brightness
    uint8_t gv = (uint8_t)green(col) / 2;
    uint8_t bv = (uint8_t)blue(col) / 2;
    strip.setPixelColor(i, strip.Color(rv, gv, bv, 0));
    yield();
  }
}

// ===================================================================================
// NAME
// birthday
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Sets all pixels to a random colour - ONLY ON BIRTHDAYS!!!
// -----------------------------------------------------------------------------------
void birthday(String dir) {
  unsigned long s_timer = millis();
  unsigned long c_timer = millis();
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  int s;
  while ( (unsigned long)(c_timer - s_timer) < limit ) {
    handleNetwork();
    for (s=1;s<=STEPS;s++) {
      setStepRndm(s, 1);
      c_timer = millis();
    }
    strip.show();
  }
  setAll(0,0,0,0);
  strip.show();
}

// ===================================================================================
// NAME
// nightAnimation
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Night mode animation (1–6 h): dim red, cascaded fade – each step starts when the
// previous one reaches ~10% brightness, so multiple steps glow simultaneously for a
// soft wave effect. bStep controls speed (brightness units per frame); overlap is the
// 10% threshold (26/255) at which the next step begins.
// gammaw[] is applied over the full 0–255 range (perceptually linear fade), then the
// result is scaled to NIGHT_BRIGHTNESS_MAX so the strip stays dim during night hours.
// -----------------------------------------------------------------------------------
void nightAnimation(String dir) {
  Serial.println("nightAnimation");
  unsigned long s_timer = millis();
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  const int bStep   = 3;   // brightness increment per frame (~3 s total fade-in)
  const int overlap = 26;  // ~10% of 255: next step starts when previous reaches this
  const int totalIter = (255 + (STEPS - 1) * overlap + bStep - 1) / bStep;

  // Cascaded fade-in
  for (int iter = 0; iter <= totalIter; iter++) {
    for (int k = 0; k < STEPS; k++) {
      int br = iter * bStep - k * overlap;
      if (br < 0) continue;
      if (br > 255) br = 255;
      int s = (dir == "UP") ? (k + 1) : (STEPS - k);
      int step_start = (s - 1) * WIDTH;
      int step_end   = step_start + WIDTH;
      uint8_t pix = (uint8_t)((uint32_t)gammaw[br] * NIGHT_BRIGHTNESS_MAX / 255);
      for (int j = step_start; j < step_end; j++)
        strip.setPixelColor(j, pix, 0, 0, 0);
    }
    strip.show();
    handleNetwork();
    yield();
  }

  // Hold
  while (millis() - s_timer < limit) {
    handleNetwork();
    delay(100);
    yield();
  }

  // Cascaded fade-out (same direction as fade-in)
  for (int iter = 0; iter <= totalIter; iter++) {
    for (int k = 0; k < STEPS; k++) {
      int br = iter * bStep - k * overlap;
      if (br < 0) continue;
      if (br > 255) br = 255;
      int s = (dir == "UP") ? (k + 1) : (STEPS - k);
      int step_start = (s - 1) * WIDTH;
      int step_end   = step_start + WIDTH;
      uint8_t pix = (uint8_t)((uint32_t)gammaw[255 - br] * NIGHT_BRIGHTNESS_MAX / 255);
      for (int j = step_start; j < step_end; j++)
        strip.setPixelColor(j, pix, 0, 0, 0);
    }
    strip.show();
    handleNetwork();
    yield();
  }

  yield();
}
// -----------------------------------------------------------------------------------


// ===================================================================================
// NAME
// fadeStep
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// This function fades all steps, each step separately, of the stairs to a given
// colour.
// -----------------------------------------------------------------------------------
void fadeStep(int red, int green, int blue, int white){
  // This function fades each step after the other to the
  // colour (red,green,blue,white)
  int i, j, s;
  for (s=1;s<=STEPS;s++) {
    // Figure out the first pixel of step s
    int step_start = (s - 1) * WIDTH;
    // Figure out the last pixel of step s
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


// ===================================================================================
// NAME
// colorWipe
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// This function fills all dots of the strip one after the other with a given color
// -----------------------------------------------------------------------------------
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
        yield();
      }
      strip.show();
    }
    delay(2000);
    for(int j = 255; j >= 0; j--){
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

