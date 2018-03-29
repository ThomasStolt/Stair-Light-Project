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
  i_1 = 255;
  for ( i = 255; i > 0; i = (int) i_1 ) {
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



void simple_fade(int val1, int val2, uint32 s_timer, uint32 c_timer, int duration){
// =========================================================================
  int count = 0, i;
  if (val1 == HIGH) {            // check if the input is HIGH
    Serial.println("UP PIR Sensor 1 Motion detected!");
    s_timer = millis();
    for ( i = 1; i <= STEPS; i++ ) {
      fadeInSingleStep(i, 100, 50, 50, 50, 50);
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
      fadeOutSingleStep(i, 100, 50, 50, 50, 50);
    }
    val2 = LOW;
    delay(7000);
  } else if ( val2 == HIGH ) {
    Serial.println("UP PIR Sensor 2 Motion detected!");
    s_timer = millis();
    for ( i = STEPS; i >= 1; i-- ) {
      fadeInSingleStep(i, 100, 50, 50, 50, 50);
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
      fadeOutSingleStep(i, 100, 50, 50, 50, 50);
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

