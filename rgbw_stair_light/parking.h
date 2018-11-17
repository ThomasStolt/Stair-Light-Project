// ===================================================================================
// FUNCTION NAME
// testPIRs
// -----------------------------------------------------------------------------------
// Continiously reads the PIRs and switches the first or last LED of the first step
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
  int count = 0, i, val1, val2;
  unsigned long s_timer = millis();
  unsigned long c_timer;
  if (dir == "UP") { // are we moving up the stairs?
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) {
      fadeInSingleStep(i, 100, 255, 255, 255, 255);
    }
    val2 = digitalRead(PIR2);
    c_timer = millis();
    // wait until either time elapsed or second PIR triggered
    while ( ! ( val2 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      delay(100);
      val2 = digitalRead(PIR2);
      yield();
      if (val2 == HIGH) {
        Serial.println("We have reached the top of the stairs or time is up!");
      }
      c_timer = millis();
    }
    // end animation
    for ( i = 1; i <= STEPS ; i++ ) {
      fadeOutSingleStep(i, 100, 255, 255, 255, 255);
    }
    val2 = LOW;
    delay(3000);
  } else if ( dir == "DOWN" ) {
    Serial.println("Moving down the stairs");
    s_timer = millis();
    for ( i = STEPS; i >= 1; i-- ) {
      fadeInSingleStep(i, 100, 255, 255, 255, 255);
    }
    val1 = digitalRead(PIR1);
    c_timer = millis();
    // wait until either time elapsed or second PIR triggered
    while ( ! ( val1 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      delay(100);
      val1 = digitalRead(PIR1);
      yield();
      if (val1 == HIGH) {
        Serial.println("We have reached the bottom of the stairs or time is up!");
      }
      c_timer = millis();
    }
    // end animation
    for ( i = STEPS; i >= 1 ; i-- ) {
      fadeOutSingleStep(i, 100, 255, 255, 255, 255);
    }
    val1 = LOW;
    delay(3000);
  }
  yield();
  delay(200);
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
  int count = 0, i, minStars = 10, maxStars = 20;
  unsigned long s_timer =  millis();
  unsigned long c_timer;
  int blue = 120;
  int blue_gamma = gammaw[blue];
  if (dir == "UP") { // We are moving up the stairs
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) { fadeInSingleStep(i, 75, 0, 0, blue, 0); }
    c_timer = millis();
    while ( ! ( digitalRead(PIR2) == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      for ( i = 1; i < random(minStars,maxStars); i++) {
        strip.setPixelColor(random(0,NUM_LEDS), 255, 255, 255, 255);
      }
      strip.show();
      setAll(0, 0, blue_gamma,0);
      strip.show();
      yield();
      if ( digitalRead(PIR2) == HIGH ) { Serial.println("We have reached the top of the stairs!"); }
      c_timer = millis();
    }
    for ( i = 1; i <= STEPS ; i++ ) { fadeOutSingleStep(i, 75, 0, 0, blue, 0); }
    delay(2500);    
  } else if ( dir == "DOWN" ) {    
    Serial.println("Moving down the stairs");
    for ( i = STEPS; i >= 1; i-- ) { fadeInSingleStep(i, 75, 0, 0, blue, 0); }
    c_timer = millis();
    while ( ! ( digitalRead(PIR1) == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      for ( i = 1; i < random(minStars,maxStars); i++) {
        strip.setPixelColor(random(0, NUM_LEDS), 255, 255, 255, 255);
      }
      strip.show();
      setAll(0, 0, blue_gamma, 0);
      strip.show();
      yield();
      if ( digitalRead(PIR1) == HIGH) { Serial.println("We have reached the bottom of the stairs!"); }
      c_timer = millis();
    }
    for ( i = STEPS; i >= 1 ; i-- ) { fadeOutSingleStep(i, 75, 0, 0, blue, 0); }
    delay(2500);
  }
  yield();
  delay(200);
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
  int count = 0, i, val1, val2;
  unsigned long s_timer = millis();
  unsigned long c_timer;
  int red = random(256);
  int green = random(256);
  int blue = random(256);
  int white = random(256);
  if (dir == "UP") { // are we moving up the stairs?
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) {
      fadeInSingleStep(i, 100, red, green, blue, white);
    }
    val2 = digitalRead(PIR2);
    c_timer = millis();
    // wait until either time elapsed or second PIR triggered
    while ( ! ( val2 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      delay(100);
      val2 = digitalRead(PIR2);
      yield();
      if (val2 == HIGH) {
        Serial.println("We have reached the top of the stairs or time is up!");
      }
      c_timer = millis();
    }
    // end animation
    for ( i = 1; i <= STEPS ; i++ ) {
      fadeOutSingleStep(i, 100, red, green, blue, white);
    }
    val2 = LOW;
    delay(3000);
  } else if ( dir == "DOWN" ) {
    Serial.println("Moving down the stairs");
    s_timer = millis();
    for ( i = STEPS; i >= 1; i-- ) {
      fadeInSingleStep(i, 100, red, green, blue, white);
    }
    val1 = digitalRead(PIR1);
    c_timer = millis();
    // wait until either time elapsed or second PIR triggered
    while ( ! ( val1 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      delay(100);
      val1 = digitalRead(PIR1);
      yield();
      if (val1 == HIGH) {
        Serial.println("We have reached the bottom of the stairs or time is up!");
      }
      c_timer = millis();
    }
    // end animation
    for ( i = STEPS; i >= 1 ; i-- ) {
      fadeOutSingleStep(i, 100, red, green, blue, white);
    }
    val1 = LOW;
    delay(3000);
  }
  yield();
  delay(200);
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
  int i, j, k;
  if (dir == "DOWN") { // are we moving down the stairs?
    Serial.println("Moving down the stairs");
    for (j=STEPS;j>=0;j--){
      fadeInSingleStep(j, 75, red(Wheel(int(((j-1)*255/STEPS)))), green(Wheel(int(((j-1)*255/STEPS)))), blue(Wheel(int(((j-1)*255/STEPS)))),0);
    }
    
    for (k=0;k<2;k++) {
      for (i=0;i<256;i=i+2){
        for (j=1;j<=STEPS;j++){
          setStep(j,Wheel(int(i + ((j-1) * 255 / STEPS))));
        }
        strip.show();
        yield();
      }
    }
    for (j=STEPS;j>=0;j--){
      fadeOutSingleStep(j, 30, red(Wheel(int(((j-1)*255/STEPS)))), green(Wheel(int(((j-1)*255/STEPS)))), blue(Wheel(int(((j-1)*255/STEPS)))),0);
    }

  } else if ( dir == "UP" ) {
    Serial.println("Moving up the stairs");

    for (j=1;j<=STEPS;j++){
      fadeInSingleStep(j, 75, red(Wheel(int(((j-1)*255/STEPS)))), green(Wheel(int(((j-1)*255/STEPS)))), blue(Wheel(int(((j-1)*255/STEPS)))),0);
    }
        
    for (k=0;k<2;k++) {
      for (i=256;i>0;i--){
        for (j=1;j<=STEPS;j++){
          setStep(j,Wheel(int(i + ((j-1) * 255 / STEPS))));
        }
        strip.show();
        yield();
      }
    }
     for (j=1;j<=STEPS;j++){
      fadeOutSingleStep(j, 30, red(Wheel(int(((j-1)*255/STEPS)))), green(Wheel(int(((j-1)*255/STEPS)))), blue(Wheel(int(((j-1)*255/STEPS)))),0);
    }
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
  int r, g, b, w;
  for(int i=step_start;i<step_end;i++){
    r = random(200);
    g = random(200);
    b = random(200);
    w = 0;
    c = Wheel(random(255));
    strip.setPixelColor(i, c);
    yield();
  }
  // strip.show();
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
  int s;
  while ( c_timer - s_timer < ANIM_DURATION ) {
  // while ( c_timer - s_timer < 15000 ) {
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

