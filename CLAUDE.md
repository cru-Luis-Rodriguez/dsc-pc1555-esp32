# Project context

Reads zone, arming and trouble state from a **DSC PC1555 (Power632)** alarm panel over
the Keybus using an **ESP32**, and exposes it as a serial stream and a self-hosted LAN
web page. No cloud, no broker, no subscription.

The panel sat unpowered for ~20 years and is currently **mid-diagnosis**. Read
`docs/panel-bringup.md` before touching anything — it is the running record of what has
been tested and ruled out.

## Hardware, confirmed from photos and measurement

| | |
|---|---|
| Panel | `PC1555 UA186 REV A`, firmware **V3.26**, manufactured wk37 **2006** |
| Family | PowerSeries (Keybus) — **not** Classic, so no `dscPC16Pin` |
| Keypads | `PC5508ZT` (8-zone LED) and a Ranger American rebrand, same layout |
| Zones in use | 5 of 8 — see zone map below |
| Battery | Original `CA1240` 12 V 4 Ah dead at 2.4 V; replacement on order |
| Powered devices | **None.** AUX and PGM terminals are empty — no PIRs on this system |

### Zone map (from the keypad label card)

| Zone | Label |
|---|---|
| 1 | Front Door / Rear Door (two contacts, one loop) |
| 2 | Master Bedroom |
| 3 | 2nd Floor |
| 4 | Rear Windows |
| 5 | Front Windows |
| 6–8 | unused |

## Current state

**PANEL RETIRED — stopping rule reached 2026-09-14.** The panel's CPU does not execute
firmware. Full record in `docs/panel-bringup.md` → "Final test session".

The end-game tests, all negative:

- **Battery test:** known-good CA1240 (12.9 V metered), battery-first-then-AC, normal
  config. Keypads unchanged.
- **Hardware factory default (§5.28, Z1↔PGM1, verified against manual):** attempted
  twice. Second attempt captured by the `probe` target across the whole 90 s power-up
  window — **0 edges/s on both lines, not one edge from AC-on**.
- **Installer lockout ruled out:** lockout announces itself with ~10 line-seizure relay
  clicks at power-up; none heard at either attempt.
- **Tap triple-verified**, so the 0-edge readings are real: pullup continuity test
  (both pins pull low through the 10 k legs), junction voltages under power (0.8 V /
  1.3 V, matching predicted 0.9 V / 1.4 V through the 0.228 divider), and the same
  GPIO 18 counted 214,552 transitions when floating.

One explanation covers everything: a CPU that isn't running can't clock the bus, can't
execute a default, and can't send the keypads valid status. Supply is fine (13.6 V);
YEL/GRN sit parked at 3.9 V / 6.2 V static.

**Decision (per the decision record): option 4 — the ESP32 reads the zone loops
directly.** See `docs/diy-zone-reader.md` → "This installation, concretely". The panel
is now a battery-backed 12 V supply and junction box. No further panel diagnostics.
Note: the zone loops themselves were never measured (Stage 1b skipped) — that
measurement is the new design's first prerequisite.

## Physical state right now — read this before diagnosing anything

| Thing | State |
|---|---|
| ESP32 | At the panel, **USB-powered**, flashed with `probe`. |
| **Keybus tap** | **Soldered and triple-verified** — YEL→33k/10k→GPIO 18, GRN→33k/10k→GPIO 19, BLK→GND. |
| Buck converter | Unopened. |
| Panel | Powered (AC + battery), normal config except **bell disconnected**. Bus dead — see Current state. |
| Battery | CA1240 installed, metered 12.9 V before install. |

> **Serial-link gotcha at the panel location:** the CH340 port intermittently wedges
> (`termios.error: (22, 'Invalid argument')` on open — only a USB replug fixes it) and
> fabricates garbage bytes during the reset window while the ESP32 TX is tristated.
> Structured lines after the app banner are always clean — judge captures by those
> only. Suspect cable/EMI; details in the bringup doc's bench note.

## Next actions, in order

The diagnostic phase is over. The project pivots to `docs/diy-zone-reader.md`:

1. **Measure the zone loops (Stage 1b)** — panel powered down, each loop at the panel
   end, P3 @ 20 kΩ. Record closed value + open/close behaviour in the bringup doc's
   measurement log. The design assumes 5.6 kΩ EOL; this is the unverified input.
2. **Buy the short parts list** — see `diy-zone-reader.md` → "Parts list — this build"
   (~$15–20: ceramics, terminal blocks, perfboard).
3. **Decide firmware**: adapt the existing `serial`/`web` targets to ADC loop reading
   (keeps the no-HA, self-hosted goal), or fork Konnected's ESPHome config (assumes
   Home Assistant). Hardware is identical either way.
4. **Build per the zone→pin map**, power the ESP32 from panel AUX through the LM2596
   (set to 5.0 V before connecting!). The Keybus tap and `probe`/`reader` targets are
   retired — keep the code; the divider parts get reused.

## Build targets

```
pio run -e reader -t upload -t monitor   # RAW Keybus dump — use this while diagnosing
pio run -e serial -t upload -t monitor   # decoded status over serial
pio run -e web    -t upload -t monitor   # self-hosted LAN page
```

Serial port on this Mac: `/dev/cu.usbserial-0001`, 115200 baud.

### Capturing serial output non-interactively

**Do not use `pio device monitor` from a script or agent session.** It requires a tty
and dies with `termios.error: (102, 'Operation not supported on socket')` the moment
its output is redirected. And macOS has no `timeout` — that's GNU coreutils, absent
unless someone brew-installed it.

Use the capture script instead:

```
~/.platformio/penv/bin/python tools/capture_serial.py /dev/cu.usbserial-0001 115200 30 out.log
```

All four arguments are optional; those are the defaults. Output goes to both stdout and
the file, so it works redirected.

Use `~/.platformio/penv/bin/python` — PlatformIO's own virtualenv, which already has
pyserial. Do **not** hardcode a Homebrew Cellar path like
`/opt/homebrew/Cellar/platformio/6.1.19_2/…`; it contains the version number and breaks
on every upgrade.

Close any interactive monitor before capturing — the port allows one reader.

The script exits 1 on zero bytes, so a zero exit already implies a non-zero byte count.
Don't check both.

### A flood of output is not success

With the tap **unwired**, GPIO 18 is a floating input. It picks up ambient coupling and
the library reads the noise as bus transitions — one bare-board capture produced
**214,552 `Keybus disconnected` lines in 20 seconds** (4.5 MB). Entirely benign, and
nothing to do with the panel.

This matters once the tap *is* wired, because **a missing ground connection produces the
same flood rather than silence.** Two hundred thousand lines scrolling past reads like
"lots of traffic, it's working." It is the opposite.

| Output | Meaning |
|---|---|
| Silence, one `Keybus disconnected` | No clock edges. Panel unpowered, or tap not connected. |
| **Flood of connect/disconnect churn** | **Floating input — ground not tied, or signal not landing** |
| Structured lines: timestamp, binary, `[hex command]`, decoded message | **Working** |

Judge by structure, not volume.

## Gotchas that will cost you time

- **Do not bump `platform = espressif32@6.9.0`.** dscKeybusInterface does not compile
  against arduino-esp32 3.x — the timer API changed. See upstream issue #344.
- **ESP32-WROOM-32 only.** The library supports esp32 and esp32-s2. Many boards sold as
  "ESP32" today are S3 or C3 and will not work.
- **ADC2 is unusable with WiFi active** — matters only for `docs/diy-zone-reader.md`,
  the fallback design. Use ADC1 (GPIO 32/33/34/35/36/39).
- **Solder the Keybus tap. Never breadboard it.** Intermittent contacts cause CRC errors
  that look exactly like a protocol fault.
- **Never disconnect a Keybus wire with the panel powered.**
- `KeybusReader` upstream is a `.ino`; `src/keybus_reader.cpp` adds `#include
  <Arduino.h>` and forward declarations so it builds as C++. Don't "fix" those.

## Docs

| File | What it covers |
|---|---|
| `docs/panel-bringup.md` | **Start here.** Staged diagnostics, all results, decision record |
| `docs/multimeter-basics.md` | Procedures P1–P4, written for a beginner; meter is a Southwire 21005N (auto-ranging, two jacks) |
| `docs/wiring.md` | Divider design, pin assignments, BOM |
| `docs/pc1555mx-programming.md` | Installer programming sections (filename says MX; the panel is a plain PC1555, but the manual sections match) |
| `docs/diy-zone-reader.md` | Fallback if the panel is unrecoverable — read zone loops directly, Konnected-style |
| `docs/build-guide.html` | Illustrated permanent install |

## Working style that has been useful here

Measure before replacing. Every hypothesis in this project has been cheap to test and
several confident-sounding ones were wrong — "both keypads lit means the CPU is running"
was wrong (keypads light on 12 V alone), and "a steady voltage means the line is idle"
was wrong (a constant-duty square wave averages to a steady number). State what a
reading would have to show to falsify a theory, then take that reading.
