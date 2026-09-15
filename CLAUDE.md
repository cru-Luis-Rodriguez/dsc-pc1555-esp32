# Project context

An **ESP32** reads the five zone loops of a retired **DSC PC1555 (Power632)** alarm
panel directly and exposes them as a serial stream and a self-hosted LAN web page. No
cloud, no broker, no subscription.

The panel sat unpowered for ~20 years and its CPU turned out to be dead — diagnosis
**concluded 2026-09-14** (`docs/panel-bringup.md` → "Final test session"). The panel
now serves only as a battery-backed 12 V supply and junction box. The active design is
`docs/diy-zone-reader.md` → "This installation, concretely"; the original Keybus
interface survives as reference code and docs.

## Hardware, confirmed from photos and measurement

| | |
|---|---|
| Panel | `PC1555 UA186 REV A`, firmware **V3.26**, manufactured wk37 **2006** |
| Family | PowerSeries (Keybus) — **not** Classic, so no `dscPC16Pin` |
| Keypads | `PC5508ZT` (8-zone LED) and a Ranger American rebrand, same layout |
| Zones in use | 5 of 8 — see zone map below |
| Battery | Original `CA1240` dead at 2.4 V; replacement CA1240 installed (12.9 V metered) |
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
3. ~~Decide firmware~~ **Decided and written**: the `serial`/`web` targets now do
   direct ADC loop reading (`src/zones.cpp` + `zone_serial.cpp`/`zone_web.cpp`),
   keeping the no-HA, self-hosted goal. Both compile. Verify/adjust the threshold
   constants in `include/zones.h` once Stage 1b numbers exist.
4. **Build per the zone→pin map**, power the ESP32 from panel AUX through the LM2596
   (set to 5.0 V before connecting!). The Keybus tap is retired — the divider parts
   get reused for zone dividers. Note the ESP32 currently has `probe` flashed and the
   old Keybus tap still soldered to GPIO 18/19; remove the tap during the rebuild.

## Build targets

```
pio run -e serial -t upload -t monitor   # zone status over serial (direct loop reading)
pio run -e web    -t upload -t monitor   # self-hosted LAN page  (direct loop reading)
```

Zone logic lives in `include/zones.h` + `src/zones.cpp` (shared by both targets);
thresholds are constants in the header, to be verified against Stage 1b measurements.

Retired Keybus targets, kept buildable for reference: `reader` (raw dump), `probe`
(edge counter), `peak` (ADC peak), `keybus-serial`, `keybus-web`.

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

### A flood of output is not success (Keybus reference targets)

Applies to the retired `reader`/`keybus-*` envs; the zone targets print fixed-format
lines and have no equivalent failure mode. With the tap **unwired**, GPIO 18 is a
floating input. It picks up ambient coupling and
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

- **ADC2 is unusable with WiFi active.** The zone reader must stay on ADC1
  (GPIO 32/33/34/35/36/39).
- **GPIO 36/39 glitch under WiFi power-save** — every WiFi target keeps
  `WiFi.setSleep(false)`; zone 5 is on GPIO 36.
- **Do not bump `platform = espressif32@6.9.0`.** dscKeybusInterface (still built by the
  reference envs) does not compile against arduino-esp32 3.x — the timer API changed.
  See upstream issue #344.
- **ESP32-WROOM-32 only.** Many boards sold as "ESP32" today are S3 or C3 and will not
  work with the pinout or the reference library.
- **Solder, never breadboard** — intermittent contacts produced phantom faults all
  through this project.
- **Never disconnect field wiring with the panel powered** — still applies; the panel
  is live as the PSU.
- `KeybusReader` upstream is a `.ino`; `src/keybus_reader.cpp` adds `#include
  <Arduino.h>` and forward declarations so it builds as C++. Don't "fix" those.

## Docs

| File | What it covers |
|---|---|
| `docs/diy-zone-reader.md` | **Start here — the active design.** "This installation, concretely": zone→pin map, power, perfboard layout, parts, wire landing |
| `docs/panel-bringup.md` | The diagnostic record: staged diagnostics, all results, final test session, decision record. Historical but load-bearing |
| `docs/multimeter-basics.md` | Procedures P1–P4, written for a beginner; meter is a Southwire 21005N (auto-ranging, two jacks) |
| `docs/wiring.md` | Retired Keybus tap design — divider math, original BOM. Reference only |
| `docs/pc1555-programming.md` | Installer programming reference. Panel retired; kept for the default procedure record |
| `docs/session-report.md` | Narrative session report from the bring-up era |
| `docs/build-guide.html` | Illustrated Keybus install guide — retired with the tap |

## Working style that has been useful here

Measure before replacing. Every hypothesis in this project has been cheap to test and
several confident-sounding ones were wrong — "both keypads lit means the CPU is running"
was wrong (keypads light on 12 V alone), and "a steady voltage means the line is idle"
was wrong (a constant-duty square wave averages to a steady number). State what a
reading would have to show to falsify a theory, then take that reading.
