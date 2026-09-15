# Session report — DSC PC1555 bring-up

Recovering a DSC PC1555 alarm panel that sat unpowered for ~20 years, and building an
ESP32 Keybus interface to read its state.

**Status at the time of writing: panel supplies power but does not clock the Keybus;
blocked on a replacement battery.** *(Since concluded — the battery and two factory
defaults changed nothing, the CPU is dead, and the project pivoted to reading the zone
loops directly. See `panel-bringup.md` → "Final test session" and
`diy-zone-reader.md`.)*

---

## 1. Headline finding

The panel's power supply works. **Nothing drives the Keybus.**

| Line | Reading | Edges/s measured |
|---|---|---|
| `RED`–`BLK` | 13.6 V | — (supply, not a signal) |
| `YEL`–`BLK` (clock) | 3.95 V steady | **0** |
| `GRN`–`BLK` (data) | 6.15 V steady | **0** |

Those voltages are **static DC levels, not duty-cycle averages.** Four separate captures
with an interrupt-driven edge counter on GPIO 18 and 19 recorded **zero transitions**,
739,913 samples per second, both pins. The same GPIO 18 counted **214,552 transitions**
when left floating on the bare board — so the pin, its interrupt and the counter all
demonstrably work.

Everything else follows from this. Two keypads receive 12 V and a pair of parked lines,
so they light up, display nothing valid, accept no keystrokes, and beep about lost
communication. The panel never reaches normal operation because it never starts talking.

---

## 2. Hardware identified

All confirmed from photographs and direct measurement, not assumption.

| | |
|---|---|
| Panel | `PC1555 UA186 REV A` — **plain PC1555, not PC1555MX** |
| Firmware | `06001489 R54CH` **V3.26** |
| Manufactured | Week 37 of **2006** (date code `0637`; UL code `061003` agrees) |
| Family | PowerSeries → Keybus, **not** Classic, so no `dscPC16Pin` |
| Transformer | 16.5 VAC 40 VA (board rated `16V AC, 50/60Hz, 2.5A max`) |
| Battery | `CA1240` 12 V 4 Ah — **dead at 2.4 V** |
| Keypad A | `PC5508ZT`, 8-zone LED, CE / RCM `N11427` |
| Keypad B | Ranger American rebrand, same PowerSeries LED layout |
| Powered devices | **None** — AUX and PGM terminals empty, so no PIRs on this system |
| Meter used | Southwire 21005N clamp meter (auto-ranging, two jacks, no frequency mode) |

### Zone map — recovered from the keypad label card

| Zone | Label | Device type |
|---|---|---|
| 1 | Front Door / Rear Door | two reed contacts, one loop |
| 2 | Master Bedroom | passive contact (no PIRs on this system) |
| 3 | 2nd Floor | passive contact |
| 4 | Rear Windows | reed contacts |
| 5 | Front Windows | reed contacts |
| 6–8 | unused | — |

---

## 3. What was tested and ruled out

Every item below was eliminated by measurement, not inference.

| Suspect | How it was ruled out |
|---|---|
| Dead board | Powers up, 13.6 V rail, keypads illuminate |
| Dead transformer | Measured at the transformer, disconnected from the panel |
| Bad power supply | 13.6 V stable under load |
| House wiring | Keypad-end readings (3.94 / 6.14 V) identical to panel-end within noise |
| Bus slot conflict | Each keypad run alone, other fully disconnected — identical behaviour |
| Either keypad individually | Both behave identically in isolation |
| Field connections | Behaviour unchanged at transformer-plus-one-keypad, zones removed one at a time |
| AUX / PGM loads | Terminals were found **empty** — nothing was ever connected |
| Programming mode | `Program` LED off |
| Fire trouble | `Fire` LED off |
| Stored alarm | `Memory` LED off |
| The ESP32 tap | Dividers verified at the pins; correct voltages arriving |
| Toolchain / firmware | Builds clean, flashes, banner prints, library initialises |
| Serial capture path | 4.5 MB captured in one run; DTR/RTS issue found and fixed |

### Panel symptoms, unchanged throughout

- `Trouble` lit; `Ready`, `Armed`, `Memory`, `Bypass`, `Fire`, `Program` all off
- **Zero zone LEDs lit** — internally inconsistent with `Ready` off, which is the tell
  that no valid status reaches the keypads
- ~1 Hz beep that `[#]` will not silence
- `[*][2]` produces no response on either keypad

---

## 4. One real fault found and fixed

**Loose black wire on the bedroom keypad** — the ground return.

An ungrounded keypad with `YEL` and `GRN` still connected sinks current through its input
protection diodes, clamping both signal lines. A genuinely sufficient cause for the
symptoms, and worth fixing.

**It was not the cause.** It was introduced during this session's rewiring, and the panel
behaved identically on the very first power-up when both keypads were lit and properly
grounded. Fixing it restored the keypad and changed nothing else.

---

## 5. What was built

### Repository

`cru-Luis-Rodriguez/dsc-pc1555-esp32` — public.

### Documentation

| File | Purpose |
|---|---|
| `docs/panel-bringup.md` | Staged diagnostic procedure, every result, decision record, stopping rule |
| `docs/multimeter-basics.md` | Written from zero for a first-time meter user; procedures P1–P4, meter-specific notes |
| `docs/wiring.md` | Divider design, pin assignments, verified BOM |
| `docs/pc1555mx-programming.md` | Installer programming sections, codes, factory-default procedure |
| `docs/diy-zone-reader.md` | Fallback design: read zone loops directly, Konnected-style |
| `docs/build-guide.html` | Illustrated permanent install |
| `CLAUDE.md` | Session handoff — hardware, state, next actions, gotchas |

### Firmware targets

| Target | Purpose |
|---|---|
| `reader` | Raw Keybus dump via `dscKeybusInterface`; works even when the panel misbehaves |
| `probe` | Interrupt edge counter on GPIO 18/19 — **the instrument that produced the headline finding** |
| `peak` | ADC min/max on GPIO 34, for measuring actual signal peaks. Built, not yet used |
| `serial` | Decoded zone/arming status (the eventual product) |
| `web` | Self-hosted LAN status page (the eventual product) |

### Tooling

`tools/capture_serial.py` — non-interactive serial capture. Exists because
`pio device monitor` requires a tty and dies when redirected, and macOS has no `timeout`.
Manages DTR/RTS explicitly (pyserial asserts them by default, which on an ESP32 drives
`EN` and `GPIO0` through the auto-reset transistors and can hold the board in reset).
Supports `--reset` to force a clean boot.

### Hardware

Two soldered 33 kΩ / 10 kΩ dividers on perfboard, verified at the ESP32 pins:

| Measurement | Expected | Measured |
|---|---|---|
| `D18` → `GND` | 10k | 9.73k |
| `D19` → `GND` | 10k | ✓ |
| `YEL` → `GND` | 43k | 44k |
| `GRN` → `GND` | 43k | ✓ |
| `3V3` → `GND` | 3.3 V | ✓ |
| `D18` → `GND` (panel live) | ~0.9 V | ✓ |
| `D19` → `GND` (panel live) | ~1.4 V | ✓ |

---

## 6. Errors made, and what corrected them

Recording these because each one cost time, and the pattern is instructive.

| Claim | Why it was wrong | What caught it |
|---|---|---|
| "Both keypads lit ⇒ the CPU is running" | Keypads illuminate on 12 V alone. `RED`/`BLK` is power; `YEL`/`GRN` is data. Power ≠ communication | Keypads showed zero zone LEDs with `Ready` off — internally inconsistent |
| **"A steady intermediate DMM reading ⇒ the line is switching at constant duty"** | A multimeter integrates. An intermediate reading is equally consistent with a line parked at that level. **This one misdirected the entire diagnosis** | The edge counter: 0 transitions |
| "PK5500 is the LCD keypad to buy" | PK-series is PC1616/1832/1864. The PC1555 needs an `LCD5500Z` | Checked before purchase |
| "The GitHub token can write to existing repos" | It has read scope only | 403 on the first write |
| "`D18` → `D19` should read `OL`" | They share a ground; the path is 10k + 10k ≈ 20k | User's measurement didn't match |
| "Lift `YEL` to float the pin as a control" | The divider's 10 kΩ still pulls the pin to ground. It cannot float | Caught before the test ran |

Two of these were mine reasoning past a measurement instead of taking one. The working
rule that emerged: **state what reading would falsify the theory, then take that
reading.** A voltmeter cannot answer "is this line moving" — that needs an edge counter.

---

## 7. Open items

| Item | Status |
|---|---|
| Probe control experiment | **Not run.** Blocked by a USB re-enumeration (`termios.error: (22, 'Invalid argument')`). Fix is unplug/replug the USB cable |
| Peak measurement on GPIO 34 | Built, not run. Would quantify how degraded the clock is, if at all |
| Replacement battery | Ordered — `CA1240` 12 V 4 Ah, F1 terminals. Not yet arrived |
| Factory default | Not attempted. Needs no keypad input; also restores installer code `5555` / master `1234` |

---

## 8. Next actions

1. **Unplug and replug the ESP32's USB cable**, then run the probe control — jumper
   `3V3` to `D18` five times during a capture. Validates the counter in place.
2. **Install the battery on arrival.** Procedure in `docs/panel-bringup.md` →
   "When the battery arrives". A panel stuck mid-initialisation would never start
   clocking, which matches what we see.
3. **Re-run `pio run -e probe`.** Nonzero edges/s means the panel started clocking.
   This is now an unambiguous pass/fail on the question that has been ambiguous since
   the first power-up.
4. **Factory default** if the battery doesn't help. Jumper Z1 to PGM1 during power-up;
   verify against manual §5.28 first. Won't work if installer lockout was set.
5. **Stopping rule:** if a good battery *and* a factory default both fail, stop. The
   decision record at the end of `docs/panel-bringup.md` explains why the expected
   return goes negative there, and what to do instead (Konnected, not a replacement
   DSC board — PowerSeries is EOL and NEO's encrypted Corbus is unsupported).

---

## 9. Cost so far

| Item | Spend |
|---|---|
| ESP32, resistors, perfboard, wire | ~$65 |
| Replacement battery | ~$20 |
| Multimeter | already owned |
| **Total** | **~$85** |

For comparison: a replacement PC1832 board was quoted at **$230** on the secondary
market, for hardware discontinued in 2022. Ruling that out early was worth more than
everything spent.

---

## 10. Assessment

The panel is not yet diagnosed to a root cause, but the fault is now **specific rather
than vague**: it supplies 13.6 V and does not clock the bus. That is a far better
position than "a 20-year-old panel doesn't work," and it was reached with a $30 meter
and one $10 microcontroller.

Two things are worth keeping regardless of how the panel ends up. The **edge counter**
is a definitive instrument for the one question that matters, and it will answer it in
15 seconds when the battery arrives. And the **tap is built and verified** — if the panel
recovers, the rest of the project is software.

The open risk is honest: a panel that supplies power but drives nothing may have a failed
processor or Keybus driver, in which case neither the battery nor the factory default will
help. The stopping rule exists for that outcome.
