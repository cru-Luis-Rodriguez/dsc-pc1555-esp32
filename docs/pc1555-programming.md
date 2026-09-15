# PC1555 / Power632 — installer programming reference

> **⛔ Panel retired 2026-09-14** — its CPU does not execute firmware, so nothing here
> can be entered anymore. Kept because the hardware-default procedure (§5.28, below)
> is part of the diagnostic record: it was attempted twice and the panel never
> performed it.

Transcribed from the DSC PC1555 (PowerSeries, 52-page) installation manual,
Section 6 Programming Worksheets. Section numbers are the same on the v2.x manual.

> **Before you touch anything:** if this panel is monitored, call the central station
> and put the account on test. Arming/disarming/alarming during programming will
> otherwise dispatch. Write down every value you change before you change it.

## Entering programming

| | |
|---|---|
| Enter installer programming | `[*][8][installer code]` |
| Default installer code | `5555` (section `[006]`) |
| Default master code | `1234` (section `[007]`) |
| Default maintenance code | `AAAA` (section `[008]`) |
| Then | key the **3-digit section number**, then the data |
| Leave a section early | `[#]` — unentered boxes keep their old values |
| Exit programming | `[#]` again |
| Hex entry | `[*]` then `1=A 2=B 3=C 4=D 5=E 6=F`, `[*]` again to return to decimal |

Filling every data box in a section exits it automatically.

## `[001]` Zone 1–8 definitions

Two digits per zone, in order: box 1 = zone 1 … box 8 = zone 8.

Factory default: `01 03 03 03 04 04 00 00`

| Code | Definition | Code | Definition |
|---|---|---|---|
| 00 | Null (not used) | 13 | 24 Hr Gas |
| 01 | Delay 1 | 14 | 24 Hr Heating (N/C) |
| 02 | Delay 2 | 15 | 24 Hr Medical |
| 03 | Instant | 16 | 24 Hr Panic |
| 04 | Interior | 17 | 24 Hr Emergency |
| 05 | Interior, Stay/Away | 18 | 24 Hr Sprinkler |
| 06 | Delay, Stay/Away | 19 | 24 Hr Water |
| 07 | Delayed 24 Hr Fire (hardwired) | 20 | 24 Hr Freeze |
| 08 | Standard 24 Hr Fire (hardwired) | 21 | 24 Hr Latching Tamper |
| 09 | 24 Hr Supervisory | 22 | Momentary Keyswitch Arm |
| 10 | 24 Hr Supervisory Buzzer | 23 | Maintained Keyswitch Arm |
| 11 | 24 Hr Burglary | 24 | LINKS Answer |
| 12 | 24 Hr Holdup | 25 | Interior Delay |
| | | 87 | Delayed 24 Hr Fire (wireless) |
| | | 88 | Standard 24 Hr Fire (wireless) |

Typical residential layout: front door `01`, back door `02`, windows/glassbreak `03`,
hallway motion `05` (so it's bypassed when armed stay), basement motion `04`.

`[002]` = zones 9–16, `[003]` = zones 17–24, `[004]` = zones 25–32 — same code table,
all default `00`. On the PC1555 these are **wireless zones only** (needs a PC5132
receiver); the panel has 6 hardwired zones on board, 8 with keypad zones.

## `[101]`–`[132]` Zone attributes

One toggle section per zone: `[101]` = zone 1 … `[132]` = zone 32.
Press the option number to flip it; the light on = enabled.

| Option | Meaning |
|---|---|
| 1 | Audible (on) / Silent (off) |
| 2 | Steady (on) / Pulsed (off) bell |
| 3 | Chime enabled |
| 4 | Bypass enabled |
| 5 | Force arming |
| 6 | Swinger shutdown |
| 7 | Transmission delay |
| 8 | Wireless zone |

Defaults vary by the zone's definition type. Option 3 is the one you want for
door-chime on entry zones.

## `[005]` System times

Four 3-digit entries, range 001–255, in this order:

| # | Entry | Units | Default |
|---|---|---|---|
| 1 | Entry Delay 1 | seconds | `030` |
| 2 | Entry Delay 2 | seconds | `045` |
| 3 | Exit Delay | seconds | `120` |
| 4 | Bell Cut-off | minutes | `004` |

## `[370]` Communication variables — **relevant to the Keybus interface**

Ten 3-digit entries, in this order:

| # | Entry | Default | Range |
|---|---|---|---|
| 1 | Swinger shutdown — alarms & restorals | `003` | 001–014, `000` = off |
| 2 | Swinger shutdown — tampers & restorals | `003` | 001–014, `000` = off |
| 3 | Swinger shutdown — maintenance & restorals | `003` | 001–014, `000` = off |
| 4 | Transmission delay | `000` | 001–255 s |
| 5 | AC failure communication delay | `030` | 001–255 min, `000` = report immediately |
| 6 | TLM trouble delay | `003` | 003–255 checks |
| 7 | Test transmission cycle (land line) | `030` | 001–255 days |
| 8 | Test transmission cycle (LINKS) | `030` | 001–255 days |
| 9 | Zone low battery transmission delay | `007` | 000–255 days |
| 10 | Delinquency transmission cycle | `030` | 000–255 |

**Why this matters:** swinger shutdown stops the panel reporting a zone after 3 alarm
events in one armed cycle — which also stops the Keybus interface seeing them. If
you're not monitored, set entries 1–3 to `000`. Setting entry 5 to `000` makes AC
power loss report immediately instead of after 30 minutes, so your web page reflects
a power cut right away.

So at the keypad: `[*][8][5555]` `370` then `000 000 000 000 000` and `[#]` to leave
the remaining five entries alone.

## `[012]` Keypad lockout

| # | Entry | Default |
|---|---|---|
| 1 | Invalid codes before lockout | `000` (disabled) |
| 2 | Lockout duration, minutes | `000` |

Both default to zero, which is exactly why the library's `Unlocker` sketch can
brute-force an unknown installer code. If you want to prevent that, set something
like `005` / `060`.

## `[016]` Fourth system option code (toggles)

| Option | Meaning | Default |
|---|---|---|
| 1 | Fire keys enabled | |
| 2 | Panic keys audible | |
| 3 | Quick exit | on |
| 4 | Quick arming (no code needed to arm) | off |
| 5 | Code required for bypassing | |
| 6 | Master code not changeable | |
| 7 | TLM enabled | |
| 8 | TLM audible | |

Option 4 on = the library's `s` / `w` virtual keypad keys arm without an access code.
Convenient; also means anything that can reach your web page can arm the system.

Option 7 off = phone-line monitoring disabled. For an unmonitored install with no
phone line this is the only way to clear the permanent trouble 3 — see the
trouble-clearing table in `panel-bringup.md` stage 5.

## Setting time and date (user function, not installer programming)

`[*][6][master code][1]`, then 10 digits: `HH MM` (24-hour, 00:00–23:59) `MM DD YY`.
Clears trouble 8 immediately. Default master code `1234`.

## Codes and resets

| Section | Purpose |
|---|---|
| `[006]` | Installer code (this is where you change `[*][8]`) |
| `[007]` | Master code |
| `[008]` | Maintenance code |
| `[990]` | **Enable** installer lockout |
| `[991]` | Disable installer lockout |
| `[999]` | `[installer code][999]` — restore factory defaults |

User access codes 1–40 are programmed by the master through `[*][5]`, not in
installer programming.

### If you don't know the installer code

1. Try `5555`, then `1555`, `1500`, `1234`.
2. Run the library's `Unlocker` sketch — on ESP32 it tries codes in order of
   statistical frequency and typically lands it in minutes to hours. It needs the
   write transistor wired.
3. Hardware default (manual §5.28, verified):
   1. Remove AC and battery.
   2. **Remove all wires from the Z1 and PGM1 terminals.**
   3. Short Z1 to PGM1 with a piece of wire.
   4. Apply **AC** power — battery alone will not default the panel.
   5. When **zone light 1** lights on the keypad, the default is complete.
   6. Remove AC, remove the jumper, restore the original wiring.

   It **wipes all programming**, and it does nothing if installer lockout (`[990]`)
   was set — the manual states a hardware default cannot be performed with lockout
   enabled. A panel with installer lockout and an unknown code cannot be recovered
   in the field.

## Sources

- [DSC PC1555 installation manual, Section 6 Programming Worksheets (pages 31–44)](https://www.manualslib.com/manual/2391841/Dsc-Pc1555.html?page=32)
- [DSC PC1555 installation manual, Section 4 How to Program (page 15)](https://www.manualslib.com/manual/2391841/Dsc-Pc1555.html?page=15)
- [DSC manual index](https://www.dsc.com/manual/29004474)
