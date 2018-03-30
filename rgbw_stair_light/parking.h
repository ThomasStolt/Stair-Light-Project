//////////////////////////////////
//  PARKING
//////////////////////////////////
//
// Measuring elapsed time in milliseconds
// uint32 t_1, t_2;
// uint32 cpu_time_used;
// t_1 = millis();
// t_2 = millis();
// Serial.print("t_1: ");
// Serial.println(t_1);
// Serial.print("t_2: ");
// Serial.println(t_2);
// Serial.print("CPU time used: ");
// Serial.println(t_2 - t_1);
//
//
// TESTING PIRs
/////////////////////////////////
// while (true) {
//     val1 = digitalRead(PIR1_PIN);  // read input value of PIR 1
//     val2 = digitalRead(PIR2_PIN);  // read input value of PIR 2
//     if ( val1 == HIGH ) {
//       strip.setPixelColor(0, 0, 200, 0, 50);
//     } else if ( val1 == LOW ) {
//       strip.setPixelColor(0, 0, 0, 0, 0);
//     }
//     if ( val2 == HIGH ) {
//       strip.setPixelColor(26, 0, 200, 0, 50);
//     } else if ( val2 == LOW ) {
//       strip.setPixelColor(26, 0, 0, 0, 0);
//     }
//     strip.show();
//     yield();
//     delay(20);
//   }

// This function fadeInSingleStep will fade a single step number step_number from zero (off)
// to the values red, green, blue within the allotted time fade_time_ms (in milliseconds)
// A reverse functionality is provided by the function fadeOutSingleStep

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
  float factor_w = float(white) / 512;
  step_width = (float) fade_time_ms / 1488;
  i_1 = 0;
  for ( i = 0; i < 256; i = (int) i_1 ) {
    i_1 = i_1 + 1 / step_width;
    for (j=step_start;j<step_end;j++) {
      strip.setPixelColor(j, gammaw[int(i*factor_r)],gammaw[int(i*factor_g)],gammaw[int(i*factor_b)], gammaw[int(i*factor_w)]);
      yield();
    }
    strip.show();
    yield();
  }
  delay(10);
}

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



void simple_fade(String dir){
  int count = 0, i, val1, val2;
  unsigned long s_timer = millis();
  unsigned long c_timer;
  int red = random(256);
  Serial.print("red: ");
  Serial.println(red);
  int green = random(256);
  Serial.print("green: ");
  Serial.println(green);  
  int blue = random(256);
  Serial.print("blue: ");
  Serial.println(blue);  
  int white = random(256);
  Serial.print("white: ");
  Serial.println(white);
  if (dir == "UP") { // are we moving up the stairs?
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) {
      fadeInSingleStep(i, 100, red, green, blue, white);
    }
    val2 = digitalRead(PIR2_PIN);
    c_timer = millis();
    // wait until either time elapsed or second PIR triggered
    while ( ! ( val2 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      delay(100);
      val2 = digitalRead(PIR2_PIN);
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
    val1 = digitalRead(PIR1_PIN);
    c_timer = millis();
    // wait until either time elapsed or second PIR triggered
    while ( ! ( val1 == HIGH || ( c_timer - s_timer > ANIM_DURATION ) ) )   {
      delay(100);
      val1 = digitalRead(PIR1_PIN);
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

