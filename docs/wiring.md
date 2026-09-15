# Wiring — PC1555 Keybus to ESP32

> **⛔ Retired 2026-09-14.** The panel's CPU is dead (`panel-bringup.md` → "Final test
> session"), so there is no Keybus to tap. The active wiring is in
> `diy-zone-reader.md` → "This installation, concretely". This doc is kept for the
> divider math and the original BOM — the ESP32, resistor kit, perfboard and buck it
> lists all carry forward to the zone reader.

## Corrections to the pin assignments you had

Your notes listed Arduino Uno pins (clock 3, data 4, keypad 5). Two things are off:

- On the Uno the library's own defaults are **clock 3, read 5, write 6**. Pin 4 is
  `dscPC16Pin`, used only for DSC *Classic* series panels — not the PC1555.
- You're on ESP32, so the pins are different again: **clock 18, read 19, write 21**.

The 33k / 10k divider values in your notes are correct for ESP32. (The 15k / 10k
pair in the library README is the Arduino-only variant.)

## Divider math

Keybus idles around 12.6 V on PowerSeries. 33k + 10k divides that to
12.6 × 10/(33+10) ≈ **2.93 V** — safe for a 3.3 V ESP32 input and comfortably above
its logic-high threshold.

## Connections

![Full wiring diagram: panel AUX through the LM2596 buck to ESP32 VIN; Yellow and Green each through a 33k/10k divider into GPIO 18 and 19; Black to ESP32 ground; optional 2N3904 from GPIO 21 pulling the Green data line low](img/keybus-esp32.svg)

| From | Via | To |
|---|---|---|
| Aux(+) | LM2596 buck, OUT set to **5.0 V** | ESP32 5V / VIN |
| Aux(−) | — | buck IN−/OUT− and ESP32 GND (common) |
| Yellow (clock) | 33 kΩ series, 10 kΩ to GND at the junction | GPIO 18 (`dscClockPin`) |
| Green (data) | 33 kΩ series, 10 kΩ to GND at the junction | GPIO 19 (`dscReadPin`) |
| GPIO 21 (`dscWritePin`) | 1 kΩ into 2N3904 base; emitter GND; collector to Green | *(optional, virtual keypad only)* |

Set the buck converter to **5.0 V with a multimeter before connecting the ESP32**.
Many LM2596 modules ship at 12 V+ out of the box and will destroy the board.

## Notes

- **Do not power an ESP32 from the panel's Aux directly.** ESP32 dev boards need the
  buck converter; only AVR Arduinos tolerate Vin straight from Aux(+).
- The PC1555 Aux output is rated **550 mA** (manual §1.1 — and one keypad's draw is
  already counted against it). An ESP32 peaks around 250 mA on WiFi TX, so
  check your existing load (keypads, motions, sirens) has headroom before adding it.
  If it's tight, power the ESP32 from a separate USB supply and tie only the grounds
  together — the divider still works, the panel just isn't sourcing the current.
- **Solder every connection.** Breadboards cause intermittent CRC errors on the Keybus.
  This is the single most common cause of "it half works" reports.
- ESP32 GPIO restrictions: use 4, 13, 16–39 for clock/read. For the write pin use
  4, 13, 16–33 only — GPIO 34–39 are input-only. Avoid pins that toggle at boot
  (0, 2, 12, 15) so you don't put spurious data on the Keybus during reset.
- **Never disconnect a Keybus wire while the panel is powered.** Kill AC and unplug
  the battery first.

## Parts

> ⚠️ **Buy an original ESP32 (ESP32-WROOM-32), not an S3/C3/C6.** dscKeybusInterface
> supports esp32 and esp32-s2 only. The S3 (Xtensa) and the RISC-V parts (C3/C6)
> have different timer peripherals and are not supported. Many boards sold today as
> "ESP32" are S3 or C3.
> Check the silkscreen on the metal can reads `ESP32-WROOM-32` (a trailing D/E/U is
> fine; `-S3` or `-C3` is not). Prefer a **CP2102** USB-UART over CH340 — macOS has
> a built-in CP210x driver, CH340 clones usually need a WCH driver on Apple Silicon.

| Part | Qty | Note |
|---|---|---|
| ESP32 dev board (NodeMCU-32S, DevKit v1, Wemos Lolin D32) | 1 | ESP32-WROOM-32 only — see warning above |
| 33 kΩ resistor | 2 | 1/4 W, any tolerance |
| 10 kΩ resistor | 2 | |
| LM2596 buck converter module | 1 | set to 5.0 V |
| 2N3904 NPN transistor | 1 | optional, virtual keypad only |
| 1 kΩ resistor | 1 | optional, transistor base |

## Full bill of materials

Amazon prices verified Aug 31 2026, Orlando delivery. Prices move — reverify.

### Required for the panel bring-up (before any ESP32 work)

| Item | Pick | Price |
|---|---|---|
| Digital multimeter | Any DMM with DC/AC volts, resistance (20k range), continuity — AstroAI or Klein entry models are fine | ~$15–35 |
| 12 V SLA battery, 4–7 Ah | Generic AGM (Mighty Max ML4-12 / ML5-12 class), F1 terminals | ~$20–25 |

The battery is not optional in practice: some PowerSeries panels won't boot on AC
alone (bring-up stage 3), and any path that keeps the panel needs one anyway. Skip
the multimeter line only if you already own one.

### Required for milestone 1 (serial over USB)

| Item | Pick | Price |
|---|---|---|
| ESP32 board | ELEGOO 3PCS ESP-32, ESP-WROOM-32, **USB-C** — 4.6★ (486) | $19.99 |
| Resistors | MelkTemn 2600 pcs / 130 values, 1% 1/4 W — 4.6★ (424) | $16.99 |
| Perfboard | ELEGOO 32 pcs double-sided PCB prototype kit — 4.8★ (2K) | $9.99 |
| Hookup wire | TUOFENG 22 AWG solid, 6 colors × 30 ft — 4.7★ (2.8K) | $15.99 |

The alternative board is **Hosyond 3-Pack, $15.99** ([B0C7C2HQ7P](https://www.amazon.com/dp/B0C7C2HQ7P),
4.6★/155) — but it is **micro-USB with no cable in the box**. On a modern Mac that
means also buying a USB-C→micro-USB *data* cable (many sold are charge-only), which
erases the $4 saving. Take the ELEGOO for the USB-C port.

On resistor kits: you need 33 kΩ ×2, 10 kΩ ×2, 1 kΩ ×1. 33k is the value cheap kits
skip — a 17-value kit will likely not have it. Buy on value count, not piece count.

### Required for permanent install

| Item | Pick | Price |
|---|---|---|
| Buck converter | HiLetgo 2pcs LM2596 with LED voltmeter — 4.3★ (764) | $10.49 |
| Heat shrink | Ginsco 580 pcs, 6 colors, 11 sizes — 4.7★ (20.9K) | $7.99 |
| Enclosure | Zulkit ABS IP65 project box, 100×68×50 mm, 2-pack — 4.7★ (3.6K) | $8.49 |

The board is 52×29×15 mm, so 100×68×50 mm fits it plus a perfboard on standoffs.

### Only if you want the virtual keypad

| Item | Pick | Price |
|---|---|---|
| NPN transistor | BOJACK 10 values / 250 pcs assortment (includes 2N3904) — 4.7★ (791) | $8.99 |

### Not needed

Powering from the panel's Aux is optional for bench work — USB from the Mac powers the
ESP32 fine as long as you still tie ESP32 GND to DSC Aux(−). The divider needs that
common ground to reference against.
