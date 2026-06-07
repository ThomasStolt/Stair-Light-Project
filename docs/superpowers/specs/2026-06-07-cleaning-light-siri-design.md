# Cleaning Light ("Staubsaugen") + Siri — Design

**Date:** 2026-06-07
**Component:** `rgbw_stair_light/rgbw_stair_light.ino` (+ README, CHANGELOG); Apple Shortcuts (user side)
**Status:** Approved

## Goal

A voice-triggered "cleaning light": "Hey Siri, Staubsaugen!" turns every LED to full
brightness for vacuuming, and it stays on for **10 minutes** or until "Hey Siri,
Staubsaugen aus!" turns it off. Brightness = **all channels at 255** (R+G+B+W max),
per user choice (brightest; note the high sustained current draw across 432 LEDs).

## Approach

Reuse the existing external-control state machine (`/api/ext`) — it already provides a
held override that suppresses motion automation and night mode while active and restores
normal operation when released. Add a new held state `clean`:

- `POST /api/ext state=clean` → all LEDs `setAll(255,255,255,255)`, held.
- Auto-off after **10 minutes** (its own timeout, distinct from the 5-min timeout used by
  the red/yellow held states). Each `clean` command resets the timer.
- `POST /api/ext state=clear` → off, override released (this is "Staubsaugen aus").

Siri integration is done with two Apple Shortcuts (no firmware involvement): a Shortcut
named **"Staubsaugen"** that POSTs `state=clean`, and **"Staubsaugen aus"** that POSTs
`state=clear`. Saying "Hey Siri, <shortcut name>" runs the matching Shortcut.

## Firmware changes

1. `enum ExtMode` gains `EXT_CLEAN`; command code `6`.
2. `#define EXT_CLEAN_TIMEOUT_MS 600000uL` (10 min).
3. `handleApiExt()` accepts `state=clean` → cmd 6 (and the help/error strings list it).
4. `applyExtCommand()` case 6: `EXT_CLEAN`, `g_extActive=true`, `setAll(255,255,255,255)`,
   `strip.show()`. (`g_extLastCmdMs` is already set for every command at the top.)
5. `serviceExtControl()` timeout guard includes `EXT_CLEAN`, using `EXT_CLEAN_TIMEOUT_MS`
   instead of the 5-min `EXT_HOLD_TIMEOUT_MS` for that mode; on timeout it clears
   (LEDs off, `EXT_NONE`, `g_extActive=false`) exactly like the other held states. `clean`
   is a solid state, so the blink/fade branches don't touch it.

No new routes; reuses `POST /api/ext`.

## Siri / Apple Shortcuts (documented for the user)

Two Shortcuts (Shortcuts app → +):
- **"Staubsaugen"**: action *Get Contents of URL* → `http://stairlight.local/api/ext`,
  Method `POST`, Request Body `Form`, field `state` = `clean`.
- **"Staubsaugen aus"**: same URL, `state` = `clear`.

"Hey Siri, Staubsaugen" / "Hey Siri, Staubsaugen aus" then trigger them. (Phone must be on
the same WiFi / reachable; `stairlight.local` resolves via mDNS, or use the IP.)

## Error handling / edge cases

- Re-sending `clean` resets the 10-min timer (keepalive).
- Works during night hours (override suppresses night mode); normal behaviour resumes on
  timeout/clear.
- `millis()` subtraction is rollover-safe.
- Power: all-channels-255 for 432 LEDs is a heavy sustained load — documented as a caution.

## Testing

Compile clean; on-device: `state=clean` → strip full white, `state=clear` → off + normal
resumes. (Full 10-min wait impractical by hand; timeout uses the same proven path as the
existing held-state timeout.)

## Out of scope

- A dedicated `/api/clean` route (reusing `/api/ext` is simpler).
- Configurable brightness/duration via the API or UI.
