# External-Control Safety Timeout — Implementation Plan

> Direct implementation (single small, mechanical change to the existing ext state machine). Compile gate: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`.

**Goal:** Held external-control states (`red`, `red_blink`, `yellow_blink`) auto-release after 5 minutes with no new `/api/ext` command, returning to normal operation.

**File:** `rgbw_stair_light/rgbw_stair_light.ino` (+ `CHANGELOG.md`, `README.md`).

## Steps

1. **Define + global.** After `bool g_extBlinkOn = false;` add:
   - `#define EXT_HOLD_TIMEOUT_MS 300000uL  // 5 min`
   - `unsigned long g_extLastCmdMs = 0;     // last /api/ext command time (for hold timeout)`
2. **Reset timer on every command.** In `applyExtCommand()`, after `unsigned long now = millis();`, add `g_extLastCmdMs = now;`.
3. **Timeout guard.** At the top of `serviceExtControl()` (after `unsigned long now = millis();`), before the blink/fade branches:
   ```cpp
   // Safety timeout: a held state auto-clears after 5 min with no new command,
   // so the stairs can't get stuck if the controller dies. green_fade is excluded
   // (it self-terminates in ~30 s).
   if (g_extMode == EXT_RED || g_extMode == EXT_RED_BLINK || g_extMode == EXT_YELLOW_BLINK) {
     if (now - g_extLastCmdMs >= EXT_HOLD_TIMEOUT_MS) {
       setAll(0, 0, 0, 0); strip.show();
       g_extMode = EXT_NONE;
       g_extActive = false;     // normal behaviour resumes next iteration
       return;
     }
   }
   ```
4. **Version:** bump `FW_VERSION` `2.3.0` → `2.4.0`.
5. **Docs:** CHANGELOG `2.4.0` entry; README external-control section notes the 5-min auto-release for held states.
6. **Compile, merge to master, OTA-flash to the device, push to GitHub.**

## Self-review

- Held states covered (red/red_blink/yellow_blink); green_fade excluded ✓.
- Timer reset on every command (measured from last command) ✓.
- Timeout path = clear semantics (LEDs off, override released, normal resumes) ✓.
- Rollover-safe unsigned subtraction ✓. No API/route changes ✓.
