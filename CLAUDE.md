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

**The panel powers up but does not clock the Keybus.**

Measured at the panel: `RED`–`BLK` **13.6 V** (supply good), `YEL`–`BLK` **3.95 V**
steady, `GRN`–`BLK` **6.15 V** steady. Identical at both keypads, so house wiring is
good.

> **Those two voltages are static DC, not duty-cycle averages.** An earlier reading of
> this project treated an intermediate DMM voltage as proof the line was switching. It
> is not — a multimeter cannot distinguish a square wave from a parked level. The
> `probe` target settles it: **0 edges/s on both GPIO 18 and 19** across four captures,
> while the same GPIO 18 counted **214,552 transitions when left floating**. The pin
> detects edges; the Keybus lines do not produce any.

So Stage 4 ("is the CPU running?") is a **FAIL**, not the pass recorded earlier. The
power supply works and nothing drives the bus.

But both keypads show **Trouble lit, Ready off, zero zone LEDs, and accept no
keystrokes**, with a ~1 Hz beep that `[#]` will not silence. Ready off with no zone
LEDs is internally inconsistent — no valid status is reaching them.

**Ruled out:** dead board · dead transformer · bad power supply · halted CPU · dead
clock · dead data line · house wiring · bus slot conflict · either keypad individually ·
every field connection (tested at transformer-plus-one-keypad).

**Remaining:** the panel's internal state — EEPROM contents, or a startup sequence that
never completes.

## Physical state right now — read this before diagnosing anything

| Thing | State |
|---|---|
| ESP32 | On the desk, **USB only**. Flashed with `reader`, verified working. |
| **Keybus tap** | **NOT WIRED.** No resistors, no connection to the panel at all. |
| Buck converter | Unopened. Not needed — USB powers the board for bench work. |
| Panel | Partly disassembled from diagnostics. Bell disconnected. Second keypad and some zone wires may still be off. Power state uncertain. |
| Battery | On order, not yet installed. |

> **`Keybus disconnected` and zero bytes is the CORRECT output in this state.** It is
> the pass condition for the bare-board test — it proves board, flash, serial link and
> library all work with nothing attached. Do not diagnose it as a fault until the tap
> is actually wired and the panel is actually powered.

## Next actions, in order

1. **Install the replacement battery when it arrives** (procedure in
   `docs/panel-bringup.md` → "When the battery arrives"). DSC panels run a startup
   battery test; a panel stuck mid-init would clock, transmit, beep on a cycle, and
   never reach normal operation — which matches exactly.
2. **`pio run -e reader -t upload -t monitor`** with the tap wired. This is the
   diagnostic that matters: raw decoded Keybus traffic, which works even when the panel
   is misbehaving. Look for whether it repeats one command forever, what the command
   byte is, and whether modules are ever acknowledged.
3. **Factory default** if the battery doesn't help — jumper Z1 to PGM1 during power-up,
   no keypad input required. Also resets installer code to `5555`, master to `1234`,
   which solves the unknown-code problem. Verify against manual §5.28 first.
4. **Stopping rule:** if a good battery *and* a factory default both fail, stop. See
   the decision record at the end of `docs/panel-bringup.md`.

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
