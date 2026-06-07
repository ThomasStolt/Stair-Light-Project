# External-Control Safety Timeout — Design

**Date:** 2026-06-07
**Component:** `rgbw_stair_light/rgbw_stair_light.ino` (+ README, CHANGELOG)
**Status:** Approved

## Goal

Prevent the stairs from getting stuck in an external-control state. The held states
(`red`, `red_blink`, `yellow_blink`) currently stay active indefinitely until the
controller sends another command. If the controller crashes or loses WiFi, the strip
would remain overridden forever. Add a **5-minute safety timeout**: if no new `/api/ext`
command arrives within 5 minutes, the override releases automatically and normal operation
(motion automation / night mode, exactly as before) resumes.

## Decisions

- **Timeout:** 5 minutes (300 s).
- **Applies to:** the held states only — `red`, `red_blink`, `yellow_blink`. `green_fade`
  already self-terminates after ~30 s, so the timeout is irrelevant to it (and not applied).
- **Reset on activity:** every received `/api/ext` command resets the 5-minute timer
  (timer is measured from the last command, not from the first).
- **On timeout:** identical to `clear` — LEDs off, `g_extActive=false`, `g_extMode=EXT_NONE`;
  `loop()` then resumes whatever normal behaviour was in effect before (automation on/off is
  never modified by external control, so "normal operations from before" is preserved).

## Implementation

In the existing external-control state machine:

1. `#define EXT_HOLD_TIMEOUT_MS 300000uL` (5 min).
2. New global `unsigned long g_extLastCmdMs = 0;` — timestamp of the most recent command.
3. `applyExtCommand()` sets `g_extLastCmdMs = millis()` for every command (resets the timer).
4. `serviceExtControl()` (already called every loop iteration while `g_extActive`) gets a
   guard at the top: if the current mode is `EXT_RED` / `EXT_RED_BLINK` / `EXT_YELLOW_BLINK`
   and `millis() - g_extLastCmdMs >= EXT_HOLD_TIMEOUT_MS`, turn LEDs off, set
   `g_extMode=EXT_NONE`, `g_extActive=false`, and return. `green_fade` is excluded.

`millis()` subtraction is rollover-safe (unsigned). No change to the API surface, routes,
or any other behaviour.

## Error handling / edge cases

- A new command mid-hold resets the timer (controller "keepalive" keeps a state active).
- `clear` and `green_fade` are unaffected (clear releases immediately; green_fade ends in 30 s).
- Rollover at ~49 days is handled by the unsigned `now - g_extLastCmdMs` idiom.

## Testing

No unit harness (firmware). Verify: compile clean; on-device, send `red`/`yellow_blink`,
confirm it holds, then send `clear` to stop (full 5-min wait is impractical to verify by
hand but the logic mirrors the proven `green_fade` auto-clear). Confirm normal operation
resumes after `clear`/timeout.

## Out of scope

- Per-command custom durations (`secs=`).
- Applying the timeout to `green_fade` (already self-terminating).
