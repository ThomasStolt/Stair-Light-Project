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
// These functions extract one colour channel from a value packed by strip.Color()
// (0xWWRRGGBB: white in bits 24-31, red 16-23, green 8-15, blue 0-7).
// -----------------------------------------------------------------------------------
uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
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

// 0 before `start`, rising linearly to 1 over `len` ms
static float ramp(uint32_t t, uint32_t start, uint32_t len) {
  if (t <= start) return 0.0f;
  if (t >= start + len) return 1.0f;
  return (float)(t - start) / len;
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
  int i;
  unsigned long s_timer = millis();
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  if (dir == "UP") {
    Serial.println("Moving up the stairs");
    for ( i = 1; i <= STEPS; i++ ) {
      fadeInSingleStep(i, 100, 255, 255, 255, 255);
    }
    while ( millis() - s_timer < limit && g_pendingExtCmd == 0 ) {
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
    while ( millis() - s_timer < limit && g_pendingExtCmd == 0 ) {
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
// Sparkling stars on a dark blue backdrop. The backdrop fades in and out as a wave
// (like nightAnimation); stars switch on at a random brightness, fade out on their own
// and also dim with their step's wave.
// -----------------------------------------------------------------------------------
void starSparkle(String dir){
  Serial.println("starSparkle");
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  bool down = (dir == "DOWN");
  Serial.println(down ? "Moving down the stairs" : "Moving up the stairs");
  const uint32_t BG = gammaw[120];   // backdrop blue at full (31 of 255)
  const uint32_t WAVE_TOTAL_MS = WAVE_FADE_MS + (STEPS - 1) * WAVE_STAGGER_MS;

  struct Star { uint16_t pixel; unsigned long born; uint16_t fadeMs; float peak; };
  const int MAX_STARS = 64;   // ~STAR_RATE * average fade time are alive at once (~22)
  Star stars[MAX_STARS];
  int nStars = 0;
  float stepE[STEPS];          // wave brightness 0..1 per step (index = step - 1)
  uint8_t stepBlue[STEPS];     // backdrop blue shown on that step this frame
  uint8_t carry[STEPS] = {};   // temporal dithering of the backdrop (it only has 31 levels)

  unsigned long t0 = millis(), last = t0, tEnd = 0;
  bool fadingOut = false;
  float due = 0;   // stars owed by the spawn rate (fractional)
  while (true) {
    unsigned long now = millis();
    if (!fadingOut && (now - t0 >= limit || g_pendingExtCmd != 0)) { fadingOut = true; tEnd = now; }
    uint32_t tIn  = (fadingOut ? tEnd : now) - t0;   // fade-in progress freezes once fade-out starts
    uint32_t tOut = fadingOut ? now - tEnd : 0;
    bool lastFrame = fadingOut && tOut >= WAVE_TOTAL_MS;   // everything at 0: draw it, then stop

    // Backdrop wave in walking order (fade-out uses the same shape as fade-in)
    for (int j = 0; j < STEPS; j++) {
      uint32_t k = down ? STEPS - 1 - j : j;
      stepE[j] = ramp(tIn, k * WAVE_STAGGER_MS, WAVE_FADE_MS)
               * (1.0f - ramp(tOut, k * WAVE_STAGGER_MS, WAVE_FADE_MS));
      uint32_t s = (uint32_t)(powf(stepE[j], 2.8f) * BG * 256.0f) + carry[j];   // 8.8 fixed point
      carry[j] = s & 0xFF;
      stepBlue[j] = s >> 8;
      uint32_t col = strip.Color(0, 0, stepBlue[j], 0);
      for (int p = j * WIDTH; p < (j + 1) * WIDTH; p++) strip.setPixelColor(p, col);
    }

    if (!fadingOut) {
      due += (now - last) * STAR_RATE / 1000.0f;
      while (due >= 1.0f && nStars < MAX_STARS) {
        due -= 1.0f;
        Star &s = stars[nStars++];
        s.pixel  = random(NUM_LEDS);
        s.born   = now;
        s.fadeMs = random(STAR_FADE_MS / 2, STAR_FADE_MS + 1);
        s.peak   = (STAR_MIN_PEAK + random(101 - STAR_MIN_PEAK)) / 100.0f;
      }
      if (due > 1.0f) due = 1.0f;   // pool full: don't burst later
    }
    last = now;

    for (int i = 0; i < nStars; ) {
      Star &s = stars[i];
      uint32_t age = now - s.born;
      if (age >= s.fadeMs) { s = stars[--nStars]; continue; }   // done: replace with last
      int j = s.pixel / WIDTH;
      float e = s.peak * (1.0f - (float)age / s.fadeMs) * stepE[j];
      uint8_t v = (uint8_t)(powf(e, 2.8f) * 255.0f + 0.5f);   // gamma 2.8, as gammaw[]
      if (v) strip.setPixelColor(s.pixel, v, v, v > stepBlue[j] ? v : stepBlue[j], v);
      i++;
    }
    strip.show();
    yield();
    if (lastFrame) break;
  }
}
// -----------------------------------------------------------------------------------



// ===================================================================================
// NAME
// wheel16
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Same colour curve as Wheel(), at 256x finer resolution: pos 0..65535 -> r,g,b in
// 8.8 fixed point (0..65280 = 0..255.0), so the rainbow flows without whole-step jumps.
// -----------------------------------------------------------------------------------
static void wheel16(uint16_t pos, uint32_t &r, uint32_t &g, uint32_t &b) {
  uint32_t q = 65535u - pos;
  if (q < 21760u) { r = 65280u - 3*q; g = 0; b = 3*q; return; }
  q -= 21760u;
  if (q < 21760u) { r = 0; g = 3*q; b = 65280u - 3*q; return; }
  q -= 21760u;
  r = (3*q > 65280u) ? 65280u : 3*q;
  g = 65280u - r;
  b = 0;
}

// ===================================================================================
// NAME
// rainbowSteps
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Flowing rainbow (one colour per step) that fades in and out as a wave in walking
// direction. Every frame is drawn the same way - current rainbow x per-step
// brightness - so there is no seam between fade-in, animation and fade-out.
// -----------------------------------------------------------------------------------
void rainbowSteps(String dir){
  Serial.println("rainbowSteps");
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  bool down = (dir == "DOWN");
  Serial.println(down ? "Moving down the stairs" : "Moving up the stairs");

  // Fade-out: a quick wave (~1.5 s), kept short so an external command shows promptly.
  const uint32_t FOUT_MS = 600;
  const uint32_t FOUT_STAGGER_MS = 60;
  const uint32_t FOUT_TOTAL_MS = FOUT_MS + (STEPS - 1) * FOUT_STAGGER_MS;

  unsigned long t0 = millis();
  unsigned long tEnd = 0;    // when the fade-out started
  bool fadingOut = false;
  uint8_t carry[STEPS][3] = {};   // temporal dithering: per step/channel remainder (1/256 level)
  while (true) {
    unsigned long now = millis();
    if (!fadingOut && (now - t0 >= limit || g_pendingExtCmd != 0)) { fadingOut = true; tEnd = now; }
    uint32_t tIn  = (fadingOut ? tEnd : now) - t0;   // fade-in progress freezes once fade-out starts
    uint32_t tOut = fadingOut ? now - tEnd : 0;
    bool lastFrame = fadingOut && tOut >= FOUT_TOTAL_MS;   // all steps at 0: draw it, then stop

    // Colours flow the whole time (also while fading); they travel in walking direction.
    uint16_t shift = (uint32_t)((now - t0) % RAINBOW_CYCLE_MS) * 65536u / RAINBOW_CYCLE_MS;
    if (!down) shift = (uint16_t)(0 - shift);

    for (int j = 1; j <= STEPS; j++) {
      uint32_t k = down ? STEPS - j : j - 1;   // position in walking order
      float e = ramp(tIn, k * RAINBOW_STAGGER_MS, RAINBOW_FADE_MS)
              * (1.0f - ramp(tOut, k * FOUT_STAGGER_MS, FOUT_MS));
      // Gamma 2.8 (the curve gammaw[] is built from), but keeping the fraction below 1
      // that the table rounds away - that's where the dithering below gets its levels.
      uint32_t L = (e >= 1.0f) ? 65280u : (e <= 0.0f) ? 0u : (uint32_t)(powf(e, 2.8f) * 65280.0f);
      uint32_t r, g, b;
      wheel16((uint16_t)(shift + (uint32_t)(j - 1) * 65536u / STEPS), r, g, b);
      uint32_t v[3] = { r * L / 65280u, g * L / 65280u, b * L / 65280u };   // still 8.8 fixed point

      // Temporal dithering: all LEDs of a step stay identical; the part below one level is
      // carried into the next frame, so e.g. 2.3 shows as 2,2,2,3,2,2,3,... (averages 2.3).
      uint8_t out[3];
      for (int c = 0; c < 3; c++) {
        uint32_t s = v[c] + (RAINBOW_DITHER ? carry[j - 1][c] : 128u);
        out[c] = s >> 8;
        if (RAINBOW_DITHER) carry[j - 1][c] = s & 0xFF;
      }
      uint32_t col = strip.Color(out[0], out[1], out[2], 0);
      int sp = (j - 1) * WIDTH;
      for (int p = 0; p < WIDTH; p++) strip.setPixelColor(sp + p, col);
    }
    strip.show();
    yield();
    if (lastFrame) break;
  }
}

// ===================================================================================
// NAME
// birthday
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// Birthday confetti: about BDAY_ON_PCT % of all LEDs glow in random colours, each
// fading in and out over a few seconds; when one goes dark another random LED starts.
// The whole picture fades in and out as a wave in walking direction (like starSparkle).
// -----------------------------------------------------------------------------------
void birthday(String dir) {
  Serial.println("birthday");
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  bool down = (dir == "DOWN");
  Serial.println(down ? "Moving down the stairs" : "Moving up the stairs");
  const uint32_t WAVE_TOTAL_MS = WAVE_FADE_MS + (STEPS - 1) * WAVE_STAGGER_MS;
  const int TARGET = NUM_LEDS * BDAY_ON_PCT / 100;

  // One light per LED (life 0 = dark). Static (~3.5 KB): too big for the stack.
  static uint32_t born[NUM_LEDS];
  static uint16_t life[NUM_LEDS];
  static uint8_t  hue[NUM_LEDS], peak[NUM_LEDS];
  memset(life, 0, sizeof(life));
  int lit = 0;

  // Start a light on a random dark LED (a few tries; at ~50% lit one is found quickly).
  // preAged: begin somewhere in its life, so the first lights don't all pulse in sync.
  auto startLight = [&](unsigned long now, bool preAged) {
    for (int tries = 0; tries < 8; tries++) {
      int p = random(NUM_LEDS);
      if (life[p]) continue;
      life[p] = random(BDAY_LIFE_MS / 2, BDAY_LIFE_MS + 1);
      born[p] = preAged ? now - random(life[p]) : now;
      hue[p]  = random(256);
      peak[p] = random(50, 101);   // % of full brightness
      lit++;
      return;
    }
  };

  unsigned long t0 = millis(), tEnd = 0;
  for (int n = 0; n < 2 * TARGET && lit < TARGET; n++) startLight(t0, true);
  bool fadingOut = false;
  float stepE[STEPS];   // wave brightness 0..1 per step (index = step - 1)
  while (true) {
    unsigned long now = millis();
    if (!fadingOut && (now - t0 >= limit || g_pendingExtCmd != 0)) { fadingOut = true; tEnd = now; }
    uint32_t tIn  = (fadingOut ? tEnd : now) - t0;   // fade-in progress freezes once fade-out starts
    uint32_t tOut = fadingOut ? now - tEnd : 0;
    bool lastFrame = fadingOut && tOut >= WAVE_TOTAL_MS;   // everything at 0: draw it, then stop

    for (int j = 0; j < STEPS; j++) {
      uint32_t k = down ? STEPS - 1 - j : j;   // position in walking order
      stepE[j] = ramp(tIn, k * WAVE_STAGGER_MS, WAVE_FADE_MS)
               * (1.0f - ramp(tOut, k * WAVE_STAGGER_MS, WAVE_FADE_MS));
    }
    for (int n = 0; n < 8 && lit < TARGET; n++) startLight(now, false);

    for (int p = 0; p < NUM_LEDS; p++) {
      if (!life[p]) { strip.setPixelColor(p, 0); continue; }
      uint32_t age = now - born[p];
      if (age >= life[p]) { life[p] = 0; lit--; strip.setPixelColor(p, 0); continue; }
      // Up over the first third of its life, down over the rest; times peak and step wave.
      float x = (float)age / life[p];
      float e = (x < 1.0f / 3) ? x * 3.0f : (1.0f - x) * 1.5f;
      e *= peak[p] / 100.0f * stepE[p / WIDTH];
      uint32_t gm = gammaw[(int)(e * 255.0f)];
      uint32_t r, g, b;
      wheel16((uint16_t)(hue[p] << 8), r, g, b);
      strip.setPixelColor(p, r * gm / 65280u, g * gm / 65280u, b * gm / 65280u, 0);
    }
    strip.show();
    yield();
    if (lastFrame) break;
  }
}

// ===================================================================================
// NAME
// matrixRain
// -----------------------------------------------------------------------------------
// SHORT DESCRIPTION
// "The Matrix" digital rain: each of the WIDTH LED positions is a column, 16 steps
// tall (all strips run the same direction, so columns line up). Drops fall from the
// top step down - always down, whichever way you walk - with a white-green head and
// a fading green trail; trail LEDs flicker now and then. A faint green base glow keeps
// the steps visible. Fades in and out as the same wave as starSparkle / birthday.
// -----------------------------------------------------------------------------------
void matrixRain(String dir) {
  Serial.println("matrixRain");
  uint32_t limit = (g_animDurationOverrideMs != 0) ? g_animDurationOverrideMs : (uint32_t)ANIM_DURATION;
  bool down = (dir == "DOWN");
  Serial.println(down ? "Moving down the stairs" : "Moving up the stairs");
  const uint32_t WAVE_TOTAL_MS = WAVE_FADE_MS + (STEPS - 1) * WAVE_STAGGER_MS;

  // One drop per column. head = distance fallen from the top step, in steps.
  struct Drop { float head; float speed; uint8_t len; unsigned long wakeAt; };
  Drop drops[WIDTH];
  auto newDrop = [](Drop &d, unsigned long now, long maxWaitMs) {
    d.head   = 0;
    d.speed  = MATRIX_DROP_SPEED * (50 + random(76)) / 100.0f;   // 0.5x..1.25x, steps per second
    d.len    = random(MATRIX_TRAIL / 2, MATRIX_TRAIL + 1);
    d.wakeAt = now + random(maxWaitMs);                           // pause before it starts falling
  };

  unsigned long t0 = millis(), last = t0, tEnd = 0;
  for (int p = 0; p < WIDTH; p++) newDrop(drops[p], t0, 2000);   // staggered start
  bool fadingOut = false;
  while (true) {
    unsigned long now = millis();
    if (!fadingOut && (now - t0 >= limit || g_pendingExtCmd != 0)) { fadingOut = true; tEnd = now; }
    uint32_t tIn  = (fadingOut ? tEnd : now) - t0;   // fade-in progress freezes once fade-out starts
    uint32_t tOut = fadingOut ? now - tEnd : 0;
    bool lastFrame = fadingOut && tOut >= WAVE_TOTAL_MS;   // everything at 0: draw it, then stop

    float dt = (now - last) / 1000.0f;
    last = now;
    for (int p = 0; p < WIDTH; p++) {
      Drop &d = drops[p];
      if ((long)(now - d.wakeAt) < 0) continue;
      d.head += d.speed * dt;
      if (d.head - d.len > STEPS) newDrop(d, now, 1500);   // trail has left the bottom step
    }

    for (int j = 0; j < STEPS; j++) {          // j = step index from the bottom
      uint32_t k = down ? STEPS - 1 - j : j;   // position in walking order (for the wave)
      float e = ramp(tIn, k * WAVE_STAGGER_MS, WAVE_FADE_MS)
              * (1.0f - ramp(tOut, k * WAVE_STAGGER_MS, WAVE_FADE_MS));
      float fade = powf(e, 2.8f);
      int fromTop = STEPS - 1 - j;
      for (int p = 0; p < WIDTH; p++) {
        Drop &d = drops[p];
        uint32_t g = MATRIX_BASE, w = 0;
        float x = d.head - fromTop;            // steps behind the head; -1..0 = head arriving
        if ((long)(now - d.wakeAt) >= 0 && x > -1.0f && x < d.len) {
          float eg = (x < 0) ? 1.0f + x : 1.0f - x / d.len;          // green: head, then fading trail
          float ew = (x < 0) ? 1.0f + x : (x < 1.0f ? 1.0f - x : 0);   // white: head only
          if (x >= 1.0f && random(1000) < 15) { eg = 1.0f; ew = 0.25f; }   // flicker
          uint32_t gv = gammaw[(int)(eg * 255.0f)];
          if (gv > g) g = gv;
          w = gammaw[(int)(ew * 255.0f)] * 55 / 100;
        }
        uint32_t rb = w * 35 / 100;
        strip.setPixelColor(j * WIDTH + p, rb * fade, g * fade, rb * fade, w * fade);
      }
    }
    strip.show();
    yield();
    if (lastFrame) break;
  }
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
    yield();
  }

  // Hold
  while (millis() - s_timer < limit && g_pendingExtCmd == 0) {
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

