# Panel bring-up and triage — PC1555 after long storage

For a PC1555 (Power632) that has been unpowered for roughly 20 years, is **not
monitored**, and is **not armed**. Goal: find out whether the board still works,
and if it doesn't, find out *which part* doesn't.

> **Why this order.** The obvious move is to wire up the ESP32 and see if anything
> comes out. Don't. If the serial output stays empty, that single symptom is
> consistent with a dead panel, a dead transformer, a miswired divider, wrong GPIO
> pins, the arduino-esp32 3.x incompatibility, a blown Aux fuse, or a panel that is
> powered but not clocking. You would have learned almost nothing. A multimeter
> separates those in fifteen minutes. **The ESP32 is stage 6, not stage 1.**

---

> ## ⛔ STATUS — CONCLUDED 2026-09-14: panel retired, CPU does not execute firmware
>
> Diagnosis is complete. Battery and factory default both failed with the tap verified;
> see **"Final test session — stopping rule reached"** below for the verdict and
> `diy-zone-reader.md` for the active replacement design. The banner below this line is
> preserved as it stood mid-diagnosis.
>
> ## ⚠️ (historical) power confirmed, communication NOT confirmed
>
> **Stage 2 and 3 pass.** On AC only with no battery, both keypads light up. The
> transformer and the panel's power supply work, and nothing on AUX is shorted (all
> field loads were still connected and it came up anyway).
>
> **Stage 4 and 5 do NOT pass, and are still open.**
>
> > **Correction to an earlier reading of this.** Keypads lighting up was initially
> > taken as proof the CPU was running. **It is not.** Red and Black supply 12 V;
> > Yellow and Green carry data. A keypad on a bus that is powered but silent will
> > show a backlight and LEDs while displaying nothing valid. Power ≠ communication.
>
> Observed on the keypad, powered:
>
> | Indicator | State |
> |---|---|
> | Trouble | **lit** |
> | Ready, Armed, Memory, Bypass, Fire, Program | all off |
> | Zone LEDs 1–8 | **all off** |
> | Audible | continuous beep ≈1/sec, **`[#]` does not silence** |
> | Keypad input | **no key registers, on either keypad** |
>
> **Ready off with zero zone LEDs lit is internally inconsistent** — an open zone
> lights its number. No valid zone status is reaching the keypads. That is the
> "backlight only, no coherent display" row of the Stage 5 table: power reaches the
> keypad, Keybus comms do not.
>
> **Next and decisive: the Stage 4 clock measurement on YEL.** It separates "power
> supply fine, CPU dead" from "CPU running, fault downstream." *(It did: 0 edges/s —
> CPU dead. See the final test session.)*
>
> `Program` off also rules out programming mode; `Fire` off rules out fire trouble.

### Zone map — from the keypad label card

| Zone | Label | Likely device |
|---|---|---|
| 1 | Front Door / Rear Door | two reed contacts in series on one loop |
| 2 | Master Bedroom | motion |
| 3 | 2nd Floor | motion |
| 4 | Rear Windows | reed contacts |
| 5 | Front Windows | reed contacts |
| 6, 7, 8 | unused | — |

**Five active zones, not six.** These are the labels to use in the web UI.

### Keypads — two, and they differ

| Location | Model | Face |
|---|---|---|
| — | `PC5508ZT` | black glossy, CE / RCM `N11427` |
| — | Ranger American rebrand, PowerSeries LED, same 8-zone layout | white / cream |

The rebrand is cosmetic — standard PowerSeries protocol underneath. Both behave
identically, which points at the panel end rather than either keypad.

## The panel in hand — identified from photos

| | |
|---|---|
| Board silkscreen | `PC1555 UA186 REV A` — **plain PC1555, not PC1555MX** |
| DSC part sticker | `70008150`, `UL 061003` |
| Main IC (U3) | `06001489 R54CH` **V3.26**, date code `0637` |
| Manufactured | Week 37 of **2006** (`0637`); UL code `061003` agrees |
| AC input rating | `16V AC, 50/60Hz, 2.5A max` = 40 VA |
| Family | DSC lists PC1555 under **PowerSeries** — Keybus, *not* Classic series |

Three consequences:

- **The wiring in `wiring.md` is correct as written.** PowerSeries means the 4-wire
  Keybus and the `dscKeybusInterface` class. No `dscPC16Pin` — that's Classic-series
  only (PC1500/PC1550/PC2550), and this isn't one.
- **This is 2006 hardware, not 1990s hardware.** Electrolytics of that vintage kept
  dry indoors usually survive. Age alone is weak evidence of failure here.
- **`PC1555` is not on the library's *tested* list** — `PC1555MX` is. The library
  states all PowerSeries panels are supported, so this should be fine, but it is the
  one genuine unknown in the stack. Keep it in mind at stage 6, not before.

> Naming note: this repo was originally written as "PC1555MX" before the board was
> identified from photos as a plain PC1555 (`UA186 REV A`). Docs and repo name now say
> PC1555. Everything MX-specific was already correct for the plain board —
> `pc1555-programming.md` was transcribed from the PC1555 manual from the start.

### Already wired, visible in the photo

The terminal strip reads `AC AC | ±AUX | +BELL− | RED BLK YEL GRN | 1 PGM 2 | 3 4`,
and **field wiring is still connected on all of it** — including all four Keybus
conductors. That means:

- **A keypad is still wired in** — a **PC5508ZT**, the 8-zone *LED* PowerSeries keypad
  (CE / RCM `N11427` international variant). Stage 5, the best diagnostic here, is
  available to you immediately at no cost. Do it.
  - Keypad-side wiring confirmed correct from photos: `Z` empty, `G`→green, `Y`→yellow,
    `B`→black, `R`→red. The `Z` terminal is the keypad zone input and is unused, so
    only the panel's onboard zones are in play.
  - Keypad PCB is in **excellent** condition — bright solder joints, no corrosion, no
    heat damage. Better preserved than the 2006 date would suggest.
  - It's an **LED** keypad, not LCD. That's ideal for go/no-go testing and painful for
    programming — see stage 5.
- **AUX and BELL have loads on them** — motions, sirens, whatever was installed. A
  shorted 20-year-old siren or a water-damaged motion will drag the supply down and
  make a perfectly healthy panel read as dead. Stage 3 has been amended accordingly.

---

## Before you start

### Tools

| Item | Need | Note |
|---|---|---|
| Digital multimeter | Required | DC volts, AC volts, resistance, continuity — **never used one? Read [`multimeter-basics.md`](multimeter-basics.md) first** |
| Screwdrivers | Required | Terminal screws are usually slotted |
| Gloves + eye protection | Required | For handling the old battery only |
| 16.5 VAC 40 VA transformer | Required | The panel's original, or a replacement |
| Known-good 12 V SLA battery | Strongly recommended | 4–7 Ah; see stage 3 |
| DSC PowerSeries keypad | Strongly recommended | Best single diagnostic you can own |
| Oscilloscope or logic analyzer | Optional | Makes stage 4 definitive rather than inferred |

### Safety

- The transformer's **mains side** is line voltage. Unplug it at the wall before
  touching its terminals. Everything downstream is low-voltage and safe to probe.
- A 20-year-old sealed lead-acid battery may have vented sulfuric acid. Gloves, eye
  protection, and don't tip it. Recycle it at any auto parts store — do not bin it.
- **Never disconnect a Keybus wire while the panel is powered.** Kill AC and unplug
  the battery first. This applies at every stage below.

### Record as you go

Write down every measurement. If you end up asking for help — or asking me — the
numbers are the whole conversation. A blank template is at the bottom of this file.

---

## Stage 1 — Visual inspection, no power

**Remove the battery before anything else**, even if you think it's fine. If the
panel has been sitting with a shorted cell, reapplying AC can push current into a
failed battery.

Check, in order:

1. **The battery itself** — swollen case, crust on the terminals, any wetness or
   staining in the cabinet below it. A bulged battery has vented; assume the cabinet
   has been exposed to acid vapor.
2. **The board near the battery leads** — green, white, or blue-green powder. This is
   the failure that actually kills stored panels. Surface crust on a terminal cleans
   up; corrosion that has wicked *under* components or along traces usually doesn't.
3. **Electrolytic capacitors** — domed tops, split vents, brown residue at the base.
4. **Burn marks, cracked components, scorched traces.**
5. **Insect or rodent damage** — nests, chewed insulation, droppings. Common in
   garage and attic installs, and urine is conductive and corrosive.
6. **Terminal screws** — loose, or corroded green.

**Pass:** board looks clean, caps look flat-topped, no corrosion spreading from the
battery area. Continue.

**Stop:** widespread corrosion crossing multiple traces, or visible burn damage. The
board *may* still be repairable, but you're now doing electronics rework, not bring-up.
A replacement PC1555 or a newer PowerSeries board is often cheaper than the hours.

---

## Stage 1b — Characterize the zone loops

**Do this even though it feels like a detour.** It tests your *sensors*, entirely
independently of whether the panel works. Every path forward — repairing the PC1555,
or replacing it with a DIY reader — depends on the door contacts and motions still
being electrically sound. If they're dead after 20 years, that reframes the whole
project before you spend a dollar.

It also tells you the end-of-line resistor value, which the DIY design in
[`diy-zone-reader.md`](diy-zone-reader.md) is built around.

**Uses procedure [P3 — resistance](multimeter-basics.md#p3--measuring-resistance-ohms).**

### Safety first

The panel must be completely dead for resistance measurements to mean anything:

1. Unplug the transformer at the wall.
2. Battery disconnected (yours already is).
3. **Verify** with [P1](multimeter-basics.md#p1--measuring-dc-volts): measure Keybus
   Red to Black. You want `0`. If you read anything, stop and find out why.

### Before you unscrew anything

**Photograph the zone terminal strip**, close up and in focus. Then label each wire
with masking tape as you remove it. Your strip reads:

```
Z1  COM  Z2   Z3  COM  Z4   Z5  COM  Z6
```

Note that **the COM terminals are shared between pairs** — Z1 and Z2 share one, Z3 and
Z4 share the next, Z5 and Z6 the last. That's normal DSC wiring, and it matters: when
you lift a COM wire you may be affecting two zones. Do one zone at a time and put it
back before moving on.

### The measurement, one zone at a time

1. Make sure **all doors and windows on that zone are closed.**
2. Loosen the screw on `Z1` and withdraw that wire. Loosen the shared `COM` and
   withdraw the wire belonging to zone 1.
3. Set the meter per **P3** — dial to `Ω`, range `20k`.
4. Touch one probe to each of the two wire ends you just removed. Hold the bare metal,
   not the insulation.
5. Read and write it down.
6. Reconnect both wires, tighten the screws, move to the next zone.

### What the reading means

| Reading | Interpretation |
|---|---|
| **~5.6 kΩ** (`5.60k`) | Healthy DSC single-EOL loop, contacts closed. **This is the ideal result.** |
| ~2.2 kΩ, ~1 kΩ, other stable value | A different EOL resistor value. Perfectly fine — just record it, the DIY design adapts. |
| **~0 Ω** (`0.00` – `0.5`) | Loop closed with **no EOL resistor** — EOL supervision was disabled. Valid, but you lose tamper detection. Note it. |
| ~11.2 kΩ | Two 5.6 kΩ in series. On DSC double-EOL (DEOL) wiring this is the *violated* state — secure DEOL reads ~5.6 kΩ, same as single-EOL. So 11.2 kΩ with the door shut means either DEOL with a contact that isn't closing, or a non-standard loop. The open/close test below distinguishes: a DEOL zone jumps 5.6 k ↔ 11.2 k instead of 5.6 k ↔ `OL`. |
| **`OL` / `1` / open** | No connection. Could be a broken wire, an open contact, **or a door on that zone genuinely standing open.** See the next test before concluding anything. |
| Very high but not open (50 kΩ+) | A corroded splice or connection. Suspect, worth chasing. |
| Drifting, won't settle | Poor probe contact — press harder — or a corroded joint in the loop. |

### The test that actually proves a sensor works

A resistance number alone doesn't tell you the contact *switches*. So:

1. With the probes still on a zone reading ~5.6 kΩ, **have someone open the door** on
   that zone (or open it yourself and watch).
2. The reading should jump to `OL` / open.
3. Close the door. It should return to ~5.6 kΩ.

**That transition is the real proof.** A sensor that changes state on demand is
working, regardless of what the panel does.

### Motion detectors will look broken — they aren't

**Expect PIR motion zones to read open with the panel unpowered.** Alarm PIRs are
designed fail-safe: they hold their relay contacts closed only while powered, so that
losing power opens the loop and trips an alarm. With no power, that relay drops out and
the zone reads open.

So:

- **Door and window contacts** are passive reed switches. They read correctly
  unpowered, and the open/close test above works on them.
- **Motion detectors and glassbreaks** are powered devices. An open reading is expected
  and is *not* evidence of a fault. You can only test these once 12 V is available —
  either from a working panel, or from the DIY build's supply.

Designs vary, so if a motion zone reads ~5.6 kΩ unpowered, that's fine too. Just don't
diagnose an open motion zone as a broken wire.

### Record it

Use the zone table in the measurement log at the bottom of this file. These numbers are
the input to every later decision.

---

## Stage 2 — Transformer, tested alone

A dead transformer mimics a dead panel exactly, and it's the cheaper failure. Rule it
out before you blame the board.

The probe positions for stages 2–5 in one picture:

![Probe map: stage 2 measures AC across the disconnected transformer leads expecting 16–19 VAC; stage 3 measures DC red-probe-on-RED black-on-BLK expecting 12.6–14 V; stage 4 moves the red probe to YEL where a wobbling reading means the CPU is clocking; stage 5 wires the keypad R B Y G to RED BLK YEL GRN](img/bringup-probe-points.svg)

**Uses procedure [P2 — AC volts](multimeter-basics.md#p2--measuring-ac-volts).**

1. **Disconnect both transformer leads from the panel's AC terminals.** Testing it
   still connected measures the panel too, which defeats the purpose.
2. Plug the transformer into the wall. **Only ever probe the two low-voltage output
   leads — never the plug end, which is line voltage.**
3. Set the meter per **P2**: dial to **AC volts** (`V~` / `ACV`), range `200`. AC has
   no polarity, so either probe on either lead.

| Reading | Meaning |
|---|---|
| ~16–19 VAC | Good. Unloaded transformers read above their rating; this is normal. |
| 0 VAC | Dead transformer, or dead outlet. Check the outlet first. |
| Well under 16 VAC | Failing. Replace it. |

4. **Unplug the transformer** before reconnecting it to the panel.

**Pass:** roughly 16–19 VAC. Continue.

**Stop:** replace the transformer before going further. Note that a dead transformer
does not clear the panel of suspicion — it just means you can't test the panel yet.

---

## Stage 3 — First power-up, AC only

### First: shed the existing loads

Your panel still has field wiring on AUX, BELL, PGM and the zones. Before the first
power-up, **label every wire, photograph the terminal strip, then disconnect**:

- **AUX+ / AUX−** — powered devices (motions, glassbreaks, keypad-adjacent hardware)
- **+BELL−** — siren or bell

**Leave the Keybus (RED BLK YEL GRN) connected.** The keypad draws very little and it
is your best diagnostic.

Why this order: a single shorted device on AUX will pull the supply down and make a
healthy panel look dead. You want the panel's supply measured unloaded first, then add
loads back one at a time. Zones can stay connected — they're sense inputs, not loads —
but if you see a low AUX reading later, they're the next thing to strip.

### Then power up

Battery still disconnected. Transformer reconnected to the panel's AC terminals, then
plugged in.

**Uses procedure [P1 — DC volts](multimeter-basics.md#p1--measuring-dc-volts).** Dial
to **DC volts** (`V⎓` / `DCV`), range `20`. **Black** probe on the Keybus **Black**
terminal, **red** probe on **Red**. Touch the metal screw head or bare wire, not the
plastic. A minus sign just means the probes are swapped.

| Reading | Meaning | Next |
|---|---|---|
| 12.6–14.0 VDC | Panel power supply works | Go to stage 4 |
| ~0 VDC | No output | See below |
| Low, e.g. 5–10 VDC | Supply loaded down or failing | Disconnect all field wiring and retest |

If you read 0 V:

- Confirm AC is actually arriving at the panel's AC terminals (meter on AC volts,
  across them, ~16–19 VAC).
- Check the onboard fuse or PTC on the Aux output. Some PowerSeries boards use a
  resettable PTC that takes a minute to recover after a fault.
- **Try adding a known-good 12 V battery.** This is the important one — some
  PowerSeries panels will not start on AC alone and need the battery present to boot.
  A panel that stays dark on AC and comes alive with a battery is *not* a dead panel.
  Treat "won't start AC-only" as inconclusive rather than a failure.

**Pass:** ~13.8 VDC on the Keybus. That means the transformer, the rectifier, and the
regulator all work. Continue.

### Before buying or connecting a battery: check the charging circuit

While the panel is up on AC with no battery, measure DC across the panel's **battery
leads** (red and black flying leads, not the Keybus): **P1**, range `20`, red probe on
the red lead. A healthy charger floats at roughly **13.6–13.8 VDC** open-circuit.

- **~13.6–13.8 V** — charging circuit works. Safe to buy and connect a fresh 12 V SLA.
- **~0 V or a few volts** — charging circuit suspect. A new battery connected here will
  just run down and die. Note it: the panel may still be usable on AC with the battery
  as a one-way reserve, but don't trust it through an outage.

Do this *before* spending on the battery, not after.

---

## Stage 4 — Is the CPU actually running?

Power present doesn't mean the processor is alive. This stage is the real question,
and it's the one people skip.

The panel continuously polls the Keybus whether or not any keypad is attached, so
clock activity is present on a healthy panel with nothing else connected.

Still on **P1 — DC volts**, range `20`.

The terminals are on the panel's main strip — the one labeled `16V AC, 50/60Hz, 2.5A
max.` at its left end:

```
┌────┬────┬─────┬─────┬──────┬──────┬─────┬─────┬─────┬─────┬────┬────┐
│ AC │ AC │ AUX+│ AUX-│ BELL+│ BELL-│ RED │ BLK │ YEL │ GRN │PGM1│PGM2│
└────┴────┴─────┴─────┴──────┴──────┴─────┴─────┴─────┴─────┴────┴────┘
                                       ▲     ▲     ▲
                                   RED─┘ BLK─┘ YEL─┘
```

> **Not** the zone `COM` terminals — those are a different thing entirely and aren't
> used here. See [`multimeter-basics.md` §0](multimeter-basics.md).

**Black** probe stays on the panel's **`BLK`** terminal; move the **red** probe to
**`YEL`**. Watch for 15–20 seconds, then repeat on **`GRN`**.

**Easier alternative:** the keypad's own terminal block is labeled `R B Y G Z` and
carries the same signals. Black probe on **`B`**, red probe on **`Y`** works just as
well and is often easier to reach.

> **Correction to an earlier version of this section.** It said "steady = dead,
> wobbling = alive." That is wrong and misleading. A multimeter averages, so a square
> wave with a *constant duty cycle* averages to a perfectly **steady** intermediate
> number. Steadiness tells you the switching is regular, not that it's absent. What
> matters is **where the reading sits**, not whether it holds still.

> ## ⚠️ This section was wrong. Read this before trusting any voltage below.
>
> An earlier version said: *"steady at an intermediate value → the line is switching
> at constant duty → the panel is clocking ✅"*. **That is not sound, and it misdirected
> this entire diagnosis.**
>
> A multimeter integrates. A steady intermediate reading is equally consistent with:
>
> 1. a square wave at constant duty cycle (line switching), **or**
> 2. a line simply parked at that DC level (not switching at all)
>
> **A DMM cannot distinguish these.** Nothing about an intermediate reading implies
> motion. Asserting otherwise produced a false "CPU running" pass at Stage 4 and cost
> an evening.
>
> **To decide it you need an edge counter, not a voltmeter.** Flash `-e probe`
> (`src/pin_probe.cpp`) and read edges/s. That is the only instrument here that
> answers the question.

| Reading | What it rules out | What it does NOT tell you |
|---|---|---|
| Steady at the rail (~13.6 V) | — | Whether the line switches |
| Steady at ~0 V | — | Whether the line switches |
| Steady at an intermediate value | Neither rail is stuck | **Nothing about switching** |
| Visibly wobbling | — | Duty cycle is at least varying |

Use the voltage only to confirm the divider ratio is sane and signal is present.
**Use `pio run -e probe` to decide whether the line is alive.**

**Measured on this panel:** `YEL` 3.95 V, `GRN` 6.15 V, both steady — and the edge
counter reports **0 edges/s on both pins** across four captures, while the same GPIO 18
counted 214,552 transitions when left floating. The pin works; the line does not move.
Those voltages are **static DC levels, not duty-cycle averages.**

**Measured on this panel:**

| Measurement | Reading | Verdict |
|---|---|---|
| `RED`–`BLK` | **13.6 V** | Power supply healthy |
| `YEL`–`BLK` | **3.95 V** steady (≈29% duty) | **Clock running** |
| `GRN`–`BLK` | **6.15 V** (≈45% duty) | **Data flowing** |

**All four Keybus conductors work at the panel.** Powered, clocking, transmitting.
The "dead board" outcome is ruled out, and so is the shorted-data-line theory.

**At the keypad terminals** (`B`→`Y`, `B`→`G`), master bedroom: **3.94 V / 6.14 V**.
Downstairs keypad: the same. Identical to the panel within measurement noise — **the
house wiring is perfect** and full-strength signal reaches both keypads.

### Physical layer: complete

| Layer | Status |
|---|---|
| Transformer | ✅ |
| Panel power supply (13.6 V) | ✅ |
| Panel CPU / clock | ✅ |
| Panel data transmission | ✅ |
| House wiring to both keypads | ✅ |
| Keypads receiving power and signal | ✅ |

Everything electrical checks out. The remaining fault is at the **protocol or
configuration level**, and multimeter diagnostics are exhausted. Stop measuring.

### Single-keypad test: negative

Each keypad was run alone, with the other fully disconnected (all four conductors) and
the panel power-cycled. **Both behaved identically** — same beep, same Trouble LED, same
dead keys.

Conclusion: **no bus slot conflict, and neither keypad is individually faulty.**

### Minimum configuration test: negative

`AUX+/AUX−` and both `PGM` terminals were found **empty** — nothing was ever connected
to them. Zone wires were then removed one at a time. Behaviour never changed at any
point, including at transformer-plus-one-keypad.

> Side finding: an empty AUX means **no powered detectors exist on this system.**
> "Master Bedroom" and "2nd Floor" are passive contacts, not PIRs. The PIR warm-up
> caveat in Stage 1b does not apply to this installation.

### Eliminated — free diagnostics complete

Dead board · dead transformer · bad power supply · halted CPU · dead clock line · dead
data line · house wiring · bus slot conflict · either keypad · every field connection.

All ruled out with a multimeter and free tests. What remains is the panel's **internal
state** — EEPROM contents, or a startup sequence that never completes. Nothing a
multimeter can reach. Further LED-watching will not resolve it.

### Battery test — run 2026-09-14, negative (see results below)

**Ordered:** Casil `CA1240`, 12 V 4 Ah SLA, F1 terminals — exact replacement for the
original, chosen to eliminate fit and terminal variables from the test.

**Panel is currently left:** bell disconnected, one keypad possibly disconnected, zones
possibly disconnected, no battery. Restore to normal config before testing.

#### When the battery arrives

1. **Meter it before installing** (P1, DC volts). Want **12.8–13.0 V** open-circuit.
   Below ~12.4 V it has sat too long — return it rather than repeat this test with a
   second bad battery.
2. **Transformer unplugged.** Reconnect the zones and the second keypad — field wiring
   is proven good. **Leave the bell disconnected** to avoid a surprise siren.
3. **Battery first, then AC.** Red to `+`, black to `−`, verify polarity, then plug in
   the transformer. The order matters: the test is whether the panel completes startup
   with a battery present.
4. Wait a few minutes, then check for any change from current behaviour:
   - Trouble LED off, or reporting only the phone line
   - Zone LEDs responding to doors opening and closing
   - **Ready** lighting with everything closed
   - `[#]` and `[*][2]` actually registering

Any one of those changing means the battery was the answer.

#### If it doesn't change anything

Go to the factory default (item 3 above), then the ESP32 bus decode (item 4), then the
stopping rule below.

### ⛔ Final test session — 2026-09-14 — stopping rule reached

Everything above was executed. Results:

**Battery test — negative.** Casil CA1240 metered **12.9 V** open-circuit before
install. Battery first, then AC, normal config (bell off). Keypads identical to
before: Trouble lit, Ready off, no zone LEDs, dead keys, cyclic beep.

**Keybus tap verified — the 0-edge readings are real.** Three independent checks:

1. *Continuity:* with the panel unpowered and internal pullups enabled, GPIO 18 and 19
   both read solid low — the 10 k legs and the ground tie are connected (an open joint
   floats high under pullup).
2. *Signal path:* with the panel powered, DMM at the divider junctions read **0.8 V**
   (GPIO 18) and **1.3 V** (GPIO 19) — matching the predicted 0.9 V / 1.4 V for YEL
   3.95 V and GRN 6.15 V through the 0.228 divider. The 33 k legs conduct.
3. *Pin function:* the same GPIO 18 counted 214,552 transitions when floating.

**Hardware factory default (§5.28) — attempted twice, negative.** AC+battery removed,
Z1 and PGM1 cleared, solid jumper, AC-only power-up. Zone light 1 never lit — though
with no valid bus data reaching the keypads, that indicator is inconclusive by itself.
The second attempt was decisive: the probe captured the entire power-up window —
**90 seconds, 0 edges/s on both clock and data, not one edge from the moment AC was
applied.** A defaulting panel would have to clock the bus afterward; this one never
clocked at all.

**Installer lockout ruled out as the blocker.** When lockout is set, a hardware
default announces the refusal: the line seizure relay chatters ~10 times (a rapid
series of clicks) right after AC is applied. **No clicks were heard at either
power-up.** That leaves one explanation covering every observation: **the CPU does
not execute firmware.** A processor that isn't running can't clock the Keybus, can't
perform a factory default (the default routine is firmware), and can't send the
keypads valid status — which is exactly the symptom set. The 13.6 V supply is fine;
the brain is gone.

**Junction voltages after all remedies:** unchanged from the original Stage 4 finding —
YEL parked at ~3.9 V, GRN at ~6.2 V, static, zero switching.

#### Decision

Known-good battery **and** factory default have both failed. Per the stopping rule and
the decision record ("Board dead → option 4"): **the PC1555 is retired as a controller.**
No further panel diagnostics. Path forward is `diy-zone-reader.md` / Konnected-style —
the ESP32 reads the zone loops directly. The Keybus house wiring is proven good and the
tap parts (ESP32, resistors, buck) carry forward. The zone loops themselves were never
measured — Stage 1b is the first prerequisite of the new design.

#### Bench note: serial capture near the panel

During this session the CH340 USB–serial link degraded when the rig moved from the
desk to the panel location: two port wedges (`termios.error: (22, 'Invalid argument')`
on open — fixed only by USB replug), one capture with 28 spurious `POWERON_RESET`
loops, and ~200 KB bursts of fabricated bytes during the reset window while the ESP32
TX is tristated (byte rate exceeded what 115200 baud can carry, so the noise is
adapter-generated, not real UART data). App output after the banner was always clean.
Judge captures by structured lines only; if the port wedges, replug USB. Suspect the
cable/EMI environment near the panel.

### Stopping rule

If the panel still behaves this way after **both** a known-good battery **and** a
factory default, stop. At that point the fault lies where only a decoded bus or a
schematic reaches, and the hours invested exceed the cost of a Konnected kit. Not
because the board is certainly bad — because the expected return has gone negative.

### What to try, cheapest first

1. **Run with only ONE keypad** (free). Disconnect the other at its `R B Y G Z`
   terminals — all four wires. DSC keypads need unique bus slots; two on the same slot
   collide, garbling keypad→panel traffic while panel→keypad broadcasts still arrive.
   That yields lit-but-unresponsive keypads on both units.
2. **Install a battery** (~$25). DSC panels run a battery test at startup and some will
   not complete initialization without one. A panel stuck mid-init would clock,
   transmit, beep on a cycle, and never reach normal operation — exactly these
   symptoms. Also needed regardless of outcome.
3. **Hardware factory default** (free). Clears possible EEPROM corruption after 20
   years *and* resets installer code to `5555` / master to `1234`, solving the unknown-
   code problem. Needs no keypad input. Wipes programming that belongs to a monitoring
   company no longer in use. Verify against manual §5.28; won't work if installer
   lockout was set.
4. **ESP32 + `KeybusReader`.** The bus carries real traffic, so decode it — is the panel
   repeating an init command forever? What trouble is it reporting? Does it acknowledge
   the keypads? Real evidence rather than inference.

### Then measure GRN — the data line

Black probe stays on `BLK`, red probe to `GRN`.

| `GRN`–`BLK` | Likely meaning |
|---|---|
| Some intermediate value | Data flowing; look at enrollment or the keypad receive side |
| Pinned at ~13.6 V | No data being transmitted |
| **Pinned at ~0 V** | **Data line shorted to ground** — explains powered-but-mute keypads |

**Isolation test if `GRN` is pinned at a rail:** power down, disconnect the field wires
from the `GRN` terminal only, power up, measure `GRN` again with nothing attached.

- Normal with wiring off → short is in the house wiring or a keypad. Reconnect one at a
  time to find it.
- Still pinned with nothing attached → the panel's data driver has failed.

Repeat on **Green (data) to Black**. Data is quieter than clock — it carries traffic in
bursts rather than continuously — so a steadier reading on Green is less alarming than
a steady reading on Yellow.

**With a scope or logic analyzer**, this becomes definitive: put a probe on Yellow,
referenced to Black, and look for periodic bursts of clock pulses repeating roughly
twice a second. Confirm the exact rate against your own capture rather than trusting
a number from memory — what matters is *periodic bursts exist*, not their precise
frequency.

**Pass:** Yellow shows activity. Continue.

**Stop:** 13.8 V present but Yellow rock-steady. The power supply works and the
processor does not. That's usually the end of the line for a board this old — but do
stage 5 first if you have a keypad, because it's a second opinion on the same question.

---

## Stage 5 — Keypad test (if you have one)

If you have any DSC PowerSeries keypad, **this is a better diagnostic than the ESP32
will ever be.** It exercises the full path — panel CPU, Keybus protocol, and
bidirectional communication — and reports results in plain language.

Power down. Wire the keypad's four leads to the matching Keybus terminals — colors
map one-to-one (see the probe-map diagram in stage 2):

| Keypad | Panel | Carries |
|---|---|---|
| `R` | Red | +12 V |
| `B` | Black | ground |
| `Y` | Yellow | clock |
| `G` | Green | data |

Power up and watch.

| What you see | Meaning |
|---|---|
| Backlight, zone lights, trouble light | **Panel is alive.** Skip to stage 6. |
| Backlight only, no coherent display | Power reaches the keypad; Keybus comms are not working |
| Completely dark | Check your four connections first, then suspect the panel |
| Any display at all | The CPU is running — that alone is the answer to stage 4 |

**Expect the trouble light to be on.** With no battery and no phone line, that's
correct behavior, not a fault.

Press `[*][2]` to list trouble conditions. On PowerSeries panels the codes are:

| Code | Trouble |
|---|---|
| 1 | Service required (press `[1]` again for the sub-code — low/missing battery lives here) |
| 2 | AC power loss |
| 3 | Telephone line trouble |
| 4 | Failure to communicate |
| 5 | Zone fault |
| 6 | Zone tamper |
| 7 | Wireless device low battery |
| 8 | Loss of time and date |

For a panel in this state, expect **1** (no battery), **3** (no phone line), and **8**
(clock lost on power-down). Those three are a *healthy* result — they're the panel
correctly reporting its situation.

### Reading trouble codes on the PC5508ZT (LED keypad)

On an LED keypad there is no text display, so `[*][2]` reports troubles by **lighting
the numbered zone LEDs 1–8** across the top. The zone LED that lights *is* the trouble
code in the table above — zone light 1 means "service required," zone light 3 means
"telephone line trouble," and so on. Multiple lights mean multiple active troubles.
Press `[#]` to exit.

So on first power-up, a healthy panel should light **zone LEDs 1, 3 and 8** under
`[*][2]`, with the Trouble LED on. That's the outcome you want.

### Clearing those troubles, one by one

| Trouble | Fix |
|---|---|
| **8** — loss of time | Set the clock: `[*][6][master code][1]`, then **10 digits**: `HH MM` (24-hour) `MM DD YY`. Default master code is `1234`. Clears immediately. |
| **1** — service required (battery) | Connect a good 12 V battery — after the charging-circuit check in stage 3. |
| **3** — telephone line | Permanent unless a phone line exists. For an unmonitored install, disable phone-line monitoring: installer programming, section `[016]`, turn **option 7 (TLM enabled)** off. This is the one trouble that never clears on its own. |

None of these block anything — the ESP32 interface works fine with the trouble light
on. Clear them for a quiet keypad, not for function.

### Reading the status LEDs on first power-up

| LED | Off | On |
|---|---|---|
| **Ready** | At least one zone is open — **expected on a fresh power-up** | All zones closed; system can be armed |
| **Armed** | Disarmed — normal | Armed, or arming |
| **Trouble** | No troubles | One or more troubles; `[*][2]` lists them |
| **Memory** | — | An alarm occurred and is stored |
| **Fire** | — | Fire zone alarm or trouble |
| **Program** | — | **In programming mode** — `[#]` twice to exit; this blocks `[*][2]` |

**Ready will be off at first, and that is not a fault.** Two causes, both benign:

- **PIR warm-up.** Motion detectors take roughly 1–3 minutes after power-up to
  stabilize and hold their zone open until they do.
- Any door or window actually standing open.

**The warm-up test:** close every door and window, leave the area so the motions see no
movement, wait three full minutes, then look. If **Ready** lights, every zone is
healthy and the sensors survived — which also means you can skip Stage 1b.

With Ready off, **the lit numbered zone LEDs 1–8 show which zones are open.** That is
the most informative readout on the keypad, and it's available without entering any
code.

### Beeping that `[#]` won't silence, and keys that do nothing

If **no** keypad command registers — `[#]` doesn't silence, `[*][2]` doesn't respond —
the problem is usually not which key you pressed. Suspect a **brownout reset loop**.

DSC keypads beep as they initialize and enroll on the Keybus. A panel that resets
repeatedly produces one beep per cycle, keypads that keep re-initializing, Ready and
Armed that never settle, and keystrokes that vanish because the panel restarts before
it can process them.

**Cause: running with no battery.** The battery absorbs current surges. On AC only,
two keypads plus motion detectors plus PGM devices can sag the rail far enough to
brown out the panel, which resets and repeats.

**Confirm it — two free checks:**

1. **Watch the keypad LEDs at the moment of each beep.** Backlight or zone LEDs
   blinking off and back = the display is re-initializing = reset loop. Settled.
2. **Measure Keybus Red-to-Black (P1, DC, range 20) while it beeps.** Steady `13.8`
   argues against brownout. A reading bouncing between ~9 and ~13 confirms it.

**The decisive test — shed load:**

1. Power down.
2. Disconnect **AUX+ / AUX−** (motions and powered devices).
3. Disconnect **one** of the two keypads, leaving a single keypad on the bus.
4. Power up.

Beeping stops and the keypad starts accepting keys → it was a power budget problem, not
a fault.

**The fix is the battery** — 12 V 7 Ah SLA, ~$25, which you need anyway.

If voltage is rock-steady and nothing re-initializes on each beep, the reset theory is
wrong; look at dead keypad membranes or a fire-zone trouble instead.

### If `[*][2]` doesn't respond

Common on first power-up, and almost always a keypad state problem rather than a fault.

1. **Press `[#]` first.** This exits whatever state the keypad is in and silences the
   trouble annunciation. Most `[*][2]` failures are the keypad sitting in another mode.
2. Press `[*]` and `[2]` as **two separate keystrokes**, not together.
3. A **single long beep** means the panel rejected the command — press `[#]` again and
   retry.
4. Remember the output is **lit zone LEDs 1–8**, not text. If you're waiting for a
   display to say something, nothing will appear to happen.
5. If the **Program** light is lit, you're in programming mode — `[#]` to exit, twice
   if needed.

If a trouble is present but you can't read the codes, the trouble itself is still real.
The Trouble LED alone plus the known context (no battery, no phone line, clock lost) is
usually enough to proceed.

### Beep patterns — what they mean

The rate matters, and it's easy to misread on first power-up:

| Pattern | Meaning |
|---|---|
| Two short beeps every ~10 seconds | **Trouble present.** Normal here. `[#]` silences it. |
| One beep per second | Exit delay counting down — the system is arming |
| Continuous / steady tone | Entry delay running |
| Rapid or temporal pattern, Memory lit | Alarm condition — **if the bell is still connected it will sound** |

> ⚠️ If you power up with the bell still wired and a 24-hour zone is faulted, the panel
> can go straight to alarm. Disconnecting `+BELL−` before first power-up (stage 3)
> avoids the surprise.

### If you plan to reprogram: get an LCD keypad

The PC5508ZT is fine for diagnostics and daily use, but **installer programming on an
LED keypad is genuinely unpleasant** — you key section numbers blind and read back
option states as lit/unlit zone LEDs, with no prompt telling you where you are. One
mis-keyed digit and you've silently changed something.

If you intend to touch programming at all — including the section `[370]` change this
project wants — a full-message LCD keypad turns blind bit-flipping into something
readable.

**The correct part for this panel is the `LCD5500Z`.**

> ⚠️ **Do not buy a PK5500, PK5501, PK5508, PK5516 or LCD5511.** Those are the *later*
> PowerSeries generation, for the PC1616 / PC1832 / PC1864. They are **not compatible
> with the PC1555**, and the PK5500 is unfortunately the one that's most prominently
> stocked on Amazon. The `PC1555RKZ` also sold there is an 8-zone *LED* keypad — no
> upgrade over the PC5508ZT already on the wall.

| | |
|---|---|
| Part | `LCD5500Z` — 64-zone programmable alphanumeric LCD |
| Requires | PC1555 / Power632 **v2.3 or higher** per DSC's compatibility listing — this panel is **V3.26** ✅. (A "v3.0+" you may see in DSC docs refers to the *keypad's own* firmware, for custom labels — not a panel requirement.) |
| Where | eBay and alarm specialty retailers; discontinued, so used / new-old-stock |
| Not on | Amazon, reliably — Amazon carries the incompatible PK-series instead |

Wire it to the same four Keybus terminals. PowerSeries panels support multiple keypads
on the bus simultaneously, so the PC5508ZT can stay in place.

**Consider skipping it.** Nothing about reading panel status over the Keybus needs
programming access — `[370]` is a refinement, not a requirement. And a newly added
keypad must itself be enrolled to a partition slot with supervision re-enabled, which
is programming, so it's partly chicken-and-egg. Confirm the panel is alive first, then
decide whether you care.

### If the keypad works but you can't get into programming

Your installer code may not be `5555`. See
[`pc1555-programming.md`](pc1555-programming.md) — it covers the code candidates,
the `Unlocker` sketch, and the hardware default jumper.

Two things to know before you go down that road:

- The `Unlocker` sketch **needs the write transistor** on GPIO 21. The read-only
  divider in [`wiring.md`](wiring.md) won't do it — that circuit deliberately cannot
  transmit.
- If a previous installer enabled installer lockout (section `[990]`) and the code
  isn't one you can guess, **the panel cannot be recovered in the field.** The hardware
  default jumper does not clear lockout. This is the one failure mode with no path
  forward, so establish it early rather than after hours of brute-forcing.

---

## Stage 6 — ESP32, raw Keybus read

Only now. Stages 3 and 4 must have passed.

Wire per [`wiring.md`](wiring.md) — clock to **GPIO 18**, data to **GPIO 19**, both
through 33k/10k dividers.

**The one thing that must be right:** ESP32 ground and panel Aux(−) must be tied
together. The divider references against that shared ground. For bench work you can
power the ESP32 from USB and connect *only* the grounds — the panel doesn't need to
source any current for this to work.

### Use KeybusReader, not the status sketch

Flash the library's **`KeybusReader`** example first, not `src/status_serial.cpp`.

KeybusReader dumps raw Keybus bytes. `status_serial.cpp` decodes those bytes into zone
and arming state and **prints nothing if decoding fails** — which reintroduces exactly
the ambiguity this whole document exists to avoid. Raw first, decoded second.

| Serial output | Meaning |
|---|---|
| Repeating command bytes, panel status lines | **Working.** Go to stage 7. |
| Garbage, inconsistent bytes, CRC errors | Wiring integrity — see below |
| Nothing at all | See the narrowed list below |

Because you've already passed stages 3 and 4, "nothing at all" now means one of only
four things, not six:

0. **Panel variant** — this is a plain `PC1555`, which is PowerSeries (so the protocol
   should match) but is not on the library's tested list. Consider this only after
   ruling out the three below, since they're far more likely.

1. **Wiring** — divider values, or clock and data swapped
2. **Wrong GPIO pins** in the sketch (must be 18 and 19 to match `wiring.md`)
3. **Toolchain** — confirm `platformio.ini` still pins `espressif32@6.9.0`.
   dscKeybusInterface does not compile against arduino-esp32 3.x
   ([upstream #344](https://github.com/taligentx/dscKeybusInterface/issues/344)),
   and a silent platform bump is an easy way to get a clean build that does nothing.

For garbage or intermittent output, it's almost always mechanical: **solder every
connection.** Breadboard contacts cause CRC errors on the Keybus, and this is the
single most common cause of "it half works."

---

## Stage 7 — Decoded status, then the web page

1. Flash `src/status_serial.cpp`. You should see zone, arming, and trouble state in
   readable form.
2. Trip a zone — open a door, or short the zone terminal to ground — and confirm the
   change appears.
3. Flash `src/web_status.cpp`, set your WiFi credentials, and load the page from
   another device on the LAN.
4. Read [`build-guide.html`](build-guide.html) for the illustrated permanent install.

Once it's working, consider section `[370]` in the programming reference — setting
swinger shutdown entries 1–3 to `000` stops the panel suppressing repeat zone events,
which otherwise makes the interface go quiet after three alarms in one armed cycle.

---

## Stage 8 — Reintroduce the shed loads

Stage 3 disconnected everything on AUX and BELL. Now that the panel is proven, put
them back **one device at a time**, with a measurement between each, so a single bad
device can't take the healthy system back down — or masquerade as a relapse.

For each device, in this order (motions first, siren last):

1. **Power down** — AC off, battery disconnected. Keybus rule applies here too.
2. Reconnect **one** device's wires to AUX (or BELL for the siren).
3. Power back up.
4. Measure Keybus Red to Black (**P1**). Still **12.6–14.0 VDC**? Good — keypad still
   lively, no new trouble under `[*][2]`? Move to the next device.
5. **Voltage sags, PTC trips, or the panel browns out** → the device you just added is
   the culprit. Disconnect it, confirm the panel recovers, and replace that device.
   A 20-year-old PIR or siren is a $15–25 part.

Notes:

- **Motions** — once powered, their zones become testable for the first time: walk-test
  each one and watch the zone open/restore on the keypad or the ESP32 serial stream.
  This completes the coverage stage 1b couldn't give you.
- **Siren** — expect nothing at idle; BELL only drives during an alarm. To test without
  waking the street, trip a zone while armed and let it sound for a second, or
  temporarily set bell cut-off (`[005]` entry 4) to its minimum first.
- **Aux budget** — the Aux supply is 550 mA with one keypad already counted. Adding the
  ESP32 (~250 mA peak) plus two or three PIRs (~15–25 mA each) fits, but if you added
  more hardware, do the arithmetic before trusting it.

---

## Triage summary

| Symptom | Most likely cause | Go to |
|---|---|---|
| No AC at panel terminals | Transformer or outlet | Stage 2 |
| AC present, 0 V on Keybus | Aux fuse/PTC, or panel supply | Stage 3 |
| Dark on AC, alive with battery | Normal for some PowerSeries | Not a fault |
| 13.8 V, Yellow rock-steady | Panel CPU not running | Stage 4 stop |
| Keypad dark, 13.8 V present | Keypad wiring, or panel comms | Stage 5 |
| Keypad lights, shows troubles 1/3/8 | **Healthy panel** | Stage 6 |
| ESP32 silent, stages 3–4 passed | Wiring, pins, or platform version | Stage 6 |
| ESP32 garbled / CRC errors | Breadboard contacts | Solder it |
| Zone reads `OL` with its door shut | Broken loop wire or dead contact | Stage 1b |
| Motion zone reads open, panel unpowered | **Expected** — PIRs fail safe | Not a fault |
| Every reading is 0 or blank | Meter setup, not the panel | `multimeter-basics.md` §2 |
| Can't enter programming | Unknown installer code | `pc1555-programming.md` |

## What usually fails on a 20-year-dormant panel

Ranked by how often it's the actual problem:

1. **The battery.** Universal. Twenty years guarantees it. Not a panel fault.
2. **The transformer.** Common, cheap, easily mistaken for a dead board.
3. **Corrosion from a vented battery.** The most common cause of genuine board death.
4. **Electrolytic capacitors.** Dry out with age; can cause brownout-like symptoms.
5. **Unknown installer code.** Not a hardware fault, but it can end the project —
   especially with lockout set.

The board itself, kept dry and not corroded, is fairly likely to still work. These
panels are simple, conservatively designed, and were built to sit powered for decades.
Age alone is not a strong reason to expect failure — moisture and battery leakage are.

---

## Upgrade paths — decision record

Two hard constraints shape every option:

1. **PowerSeries NEO (HS2016/HS2032/HS2064/HS2128) is unusable for this project.** NEO
   replaced the Keybus with an *encrypted* Corbus. `dscKeybusInterface` cannot support
   it and says so. Buying NEO means buying the newest DSC hardware and losing the whole
   reason for the ESP32.
2. **PC1616 / PC1832 / PC1864 are discontinued** — DSC ended them 31 March 2022, "while
   quantities last." Secondary market only; a PC1832 V4.6 was listed at ~$230.

| Option | Cost | Keeps this project? | Verdict |
|---|---|---|---|
| 1. Keep PC1555 + ESP32 | ~$50–70 parts + $25 battery | Yes, as designed | **Do this if the board works** |
| 2. Newer PowerSeries board | ~$150–230, +$60–90 for a PK5500 | Yes; on the tested list | **No** — premium for 2022-EOL hardware |
| 3. PowerSeries NEO | New retail | **No** — encrypted Corbus | **No** |
| 4. Konnected (ESP32 + ESPHome) | Kit, on Amazon | Replaces it | **Do this if the board is dead** |
| 5. DIY — ESP32 reads zone loops direct | ~$20–30 | Replaces it | Viable; option 4 with more work |

### Decision rule

**Board works → option 1. Board dead → option 4. Never option 2 or 3.**

Supporting reasoning:

- **Option 1's parts aren't stranded.** Konnected *is* an ESP32 running ESPHome, so the
  ESP32, resistors, buck converter and perfboard carry forward if you change direction
  later. Only the battery is DSC-specific, and every DSC path needs it.
- **A working panel is free alarm behavior.** Siren driving, entry/exit delays and
  arming logic already exist in firmware. Keeping a live board preserves the option to
  want a real alarm later at no cost — which is what you want while undecided.
- **"Modern integration" is not a reason to leave DSC.** `dscKeybusInterface` ships
  Home Assistant and MQTT examples, so HA entities, automations and a local dashboard
  are available on the 2006 board for $0 extra.

### What none of this is

No path here is a *security system*: no supervised communications, no central-station
monitoring, no UL listing, no tamper or line-cut response. A working PC1555 with a
siren gets closest. If the actual goal is protecting the house rather than telemetry
and automation, the answer is a monitored system — which means a subscription, and a
different set of trade-offs than this repo is built around.

---

## Measurement log

```
Date: ____________

Stage 1  Visual
  Battery condition ......................... ____________________
  Corrosion near battery leads .............. ____________________
  Capacitors ................................ ____________________
  Other damage .............................. ____________________

Stage 1b  Zone loops   (panel dead, doors/windows closed, P3 @ 20k)
  Meter proved on a known battery first? .... Y / N
                        closed      opens when triggered?   device type
  Zone 1 ............ __________     Y / N / n-a           ____________
  Zone 2 ............ __________     Y / N / n-a           ____________
  Zone 3 ............ __________     Y / N / n-a           ____________
  Zone 4 ............ __________     Y / N / n-a           ____________
  Zone 5 ............ __________     Y / N / n-a           ____________
  Zone 6 ............ __________     Y / N / n-a           ____________

  EOL resistor value observed ............... __________ ohms
  (device type: reed contact / motion PIR / glassbreak / unknown)
  (motions reading open with no power is EXPECTED - not a fault)

Stage 2  Transformer (disconnected from panel)
  AC across leads ........................... __________ VAC

Stage 3  Power-up, AC only
  AC at panel terminals ..................... __________ VAC
  Keybus Red to Black ....................... __________ VDC
  Started without battery? .................. Y / N
  Battery leads, open-circuit (charger) ..... __________ VDC  (want ~13.6-13.8)

Stage 4  CPU
  Yellow to Black ........................... __________ VDC
  Steady or fluctuating? .................... ____________________
  Green to Black ............................ __________ VDC
  Scope: periodic bursts seen? .............. Y / N / n/a

Stage 5  Keypad
  Keypad model .............................. ____________________
  Display ................................... ____________________
  [*][2] trouble codes ...................... ____________________

Stage 6  ESP32
  KeybusReader output ....................... ____________________
  platformio.ini platform ................... ____________________

Stage 8  Loads back on
  Device 1 ______________ reconnected → Keybus __________ VDC   OK? Y / N
  Device 2 ______________ reconnected → Keybus __________ VDC   OK? Y / N
  Device 3 ______________ reconnected → Keybus __________ VDC   OK? Y / N
  Siren (BELL) reconnected → Keybus __________ VDC   sounded on test? Y / N
  Motion walk-test: each zone opens/restores? .. Y / N
```
