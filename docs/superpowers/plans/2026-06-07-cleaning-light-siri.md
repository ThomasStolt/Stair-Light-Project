# Cleaning Light ("Staubsaugen") + Siri — Implementation Plan

> Direct implementation. Adds a `clean` held state to the existing `/api/ext` machine with a 10-minute auto-off. Compile gate: `arduino-cli compile --fqbn esp8266:esp8266:nodemcu rgbw_stair_light`.

**File:** `rgbw_stair_light/rgbw_stair_light.ino` (+ `CHANGELOG.md`, `README.md`). Siri side = Apple Shortcuts (documented, no code).

## Steps

1. **Enum + command code.** `enum ExtMode { ... , EXT_YELLOW_BLINK };` → append `, EXT_CLEAN`. Update the `g_pendingExtCmd` comment to add `,6=clean`.
2. **Timeout define.** After `#define EXT_HOLD_TIMEOUT_MS 300000uL ...` add `#define EXT_CLEAN_TIMEOUT_MS 600000uL // clean (full brightness) auto-clears after 10 min`.
3. **Handler.** In `handleApiExt()`: add `else if (s == F("clean")) cmd = 6;`; add `clean` to the comment and both 400 help strings (`state=red|red_blink|green_fade|yellow_blink|clean|clear`).
4. **applyExtCommand().** Add before `case 4`:
   ```cpp
   case 6: // clean – all channels full brightness, held (10 min timeout)
     g_extMode = EXT_CLEAN; g_extActive = true;
     setAll(255, 255, 255, 255); strip.show();
     break;
   ```
5. **serviceExtControl() timeout guard.** Replace the held-state timeout block with one that includes `EXT_CLEAN` and uses a per-mode timeout:
   ```cpp
   if (g_extMode == EXT_RED || g_extMode == EXT_RED_BLINK ||
       g_extMode == EXT_YELLOW_BLINK || g_extMode == EXT_CLEAN) {
     unsigned long holdTimeout = (g_extMode == EXT_CLEAN) ? EXT_CLEAN_TIMEOUT_MS : EXT_HOLD_TIMEOUT_MS;
     if (now - g_extLastCmdMs >= holdTimeout) {
       setAll(0, 0, 0, 0); strip.show();
       g_extMode = EXT_NONE;
       g_extActive = false;
       return;
     }
   }
   ```
6. **Version:** `FW_VERSION` `2.4.0` → `2.5.0`.
7. **Docs:** CHANGELOG `2.5.0` entry; README — add `clean` to the `/api/ext` table + a "Siri / Apple Shortcuts" subsection with the two Shortcut definitions.
8. **Compile, commit, OTA-flash to device, push to GitHub.**

## Self-review

- `clean` = all-channels-255, held, 10-min auto-off; `clear` = off ("Staubsaugen aus") ✓.
- Per-mode timeout (10 min for clean, 5 min for others) ✓; timer reset on every command ✓.
- Solid state — not touched by blink/fade branches ✓. Rollover-safe ✓. No new routes ✓.
- Siri documented as two Shortcuts POSTing `state=clean` / `state=clear` ✓.
