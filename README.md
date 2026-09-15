# ESP32 zone reader — for a retired DSC PC1555

An ESP32 reads the five zone loops of a DSC PC1555 (Power632) alarm panel directly and
exposes them — first over USB serial, then as a self-hosted web page on your LAN. No
cloud, no broker, no subscription.

**Why direct reading:** the panel sat unpowered for ~20 years and its CPU turned out
to be dead. A known-good battery and two hardware factory defaults changed nothing,
with an edge-counter probe watching the Keybus produce zero transitions through the
entire power-up window. The full diagnosis — and the reasoning at every step — is in
[`docs/panel-bringup.md`](docs/panel-bringup.md). The panel now serves as a
battery-backed 12 V supply and junction box; the ESP32 does the sensing.

This repo started as a Keybus interface (read the panel's bus, keep the panel in
charge). That design was never installed — it survives as reference code and docs,
marked ⛔ where retired.

## Docs

| Doc | What |
|---|---|
| [`docs/diy-zone-reader.md`](docs/diy-zone-reader.md) | **The active design — start here.** "This installation, concretely": zone→pin map, perfboard layout, power tree, parts list with links |
| [`docs/panel-bringup.md`](docs/panel-bringup.md) | The diagnostic record: staged tests, every result, the stopping rule, the decision record |
| [`docs/multimeter-basics.md`](docs/multimeter-basics.md) | **Never used a multimeter?** procedures P1–P4 that the other docs reference |
| [`docs/wiring.md`](docs/wiring.md) | ⛔ Retired Keybus tap design — divider math and original BOM (parts carry forward) |
| [`docs/pc1555-programming.md`](docs/pc1555-programming.md) | ⛔ Installer programming reference — kept for the factory-default record |
| [`docs/build-guide.html`](docs/build-guide.html) | ⛔ Illustrated Keybus install guide — retired with the tap |

## Status

- **Diagnosis:** concluded 2026-09-14 — panel retired, decision record written.
- **Firmware:** written and compiling — `serial` and `web` targets read the loops via
  ADC1 (`include/zones.h`, `src/zones.cpp`); thresholds await verification.
- **Hardware:** pending — measure the zone loops (Stage 1b in the bring-up doc), then
  build per the perfboard layout. Not yet tested against live loops.

## Build targets

```bash
pio run -e serial -t upload -t monitor   # zone status over serial
pio run -e web    -t upload -t monitor   # self-hosted LAN page
```

For the web target first:

```bash
cp include/secrets.h.example include/secrets.h
$EDITOR include/secrets.h                # WiFi SSID + password
```

Zone names and pins live in `src/zones.cpp`; classification thresholds in
`include/zones.h` — verify them against the bench voltages the serial target prints.

Expected serial output once loops are wired:

```
zone reader: direct loop sensing on ADC1 (no panel)
thresholds mV: short<700  normal 900-1900  open>=2000
---- zone snapshot ----
zone 1  Front/Rear Door  GPIO32  1390 mV  closed
...
[   123456 ms] zone 1  Front/Rear Door  closed -> OPEN  (2410 mV)
```

Retired Keybus targets stay buildable for reference: `reader` (raw bus dump), `probe`
(edge counter), `peak` (ADC peak), `keybus-serial`, `keybus-web`.

The web page has **no authentication** — it is read-only status, but still: LAN or an
IoT VLAN, not the open internet.

## macOS setup

```bash
brew install platformio            # or: use the PlatformIO IDE extension in VS Code
```

Find the board's serial port after plugging it in:

```bash
ls /dev/cu.*
```

You want `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART` (CP2102) or
`/dev/cu.wchusbserial*` (CH340). If nothing new appears, the board's USB-UART chip
needs a driver — CH340 clones usually do on Apple Silicon. If PlatformIO picks the
wrong port, add `upload_port = /dev/cu.usbserial-0001` to `platformio.ini`.

For scripted/agent captures use `tools/capture_serial.py`, not `pio device monitor` —
the monitor needs a tty and dies when redirected. Known bench gremlin: the CH340
driver on macOS occasionally wedges (`termios` error 22 on open); a USB replug clears
it.

> **Do not upgrade the ESP32 platform.** `platformio.ini` pins `espressif32@6.9.0`
> (arduino-esp32 2.0.x) because dscKeybusInterface — still built by the reference
> envs — does not compile against arduino-esp32 3.x
> ([issue #344](https://github.com/taligentx/dscKeybusInterface/issues/344)).

## The Keybus era (reference)

Everything below applied to the original plan — reading a *working* panel's bus. Kept
because the reference envs still build it, and because it's the road map if this code
ever meets a healthy PowerSeries panel.

### Upstream maintenance status

Checked 2026-09-06:

| Repo | Last commit | State |
|---|---|---|
| [taligentx/dscKeybusInterface](https://github.com/taligentx/dscKeybusInterface) | **2022-03-18** ("Release 3.0") | Frozen. 49 open issues, recent ones unanswered. |
| [Dilbert66/esphome-components](https://github.com/Dilbert66/esphome-components) | **2026-09-05** | Active — merging outside PRs. |
| [Dilbert66/esphome-dsckeybus](https://github.com/Dilbert66/esphome-dsckeybus) | **2026-08-24** | Active. |

The upstream library has been unmaintained for four and a half years. Less alarming
than it sounds: the Keybus protocol is a frozen target, so the decoding logic doesn't
rot — only the boundary with the ESP32 toolchain does, which the platform pin handles.
None of upstream's forks became a maintained successor; Dilbert66's ESPHome component
is the real one (and its `Mqtt_Example/` is plain Arduino, so it's reachable without
ESPHome).

### Toolchain options for a working panel

| Option | Trade-off |
|---|---|
| **Pin `espressif32@6.9.0`** (this repo) | Simplest. You write C++ and own every line. Toolchain frozen. |
| **[Dilbert66/esphome-dsckeybus](https://github.com/Dilbert66/esphome-dsckeybus)** | Actively maintained fork as an ESPHome component — tracks current esp32 core, adds zone-expander emulation, PGM outputs, panel time sync. YAML instead of C++. Pins: clock 22, read 21, write 18. |
| **Patch the timer calls yourself** | ~20 lines in the library's esp32 timer setup. Frees the pin; you maintain a fork. |
| **Envisalink 4** | Commercial board, no soldering, HA out of the box. ~10× the cost. |

The ESPHome route does **not** require Home Assistant — its `web_server` component
serves a standalone LAN page.

## References

- [taligentx/dscKeybusInterface](https://github.com/taligentx/dscKeybusInterface) — the reference-env library
- [Issue #344 — arduino-esp32 3.x incompatibility](https://github.com/taligentx/dscKeybusInterface/issues/344)
- [DSC PC1555 installation manual](https://www.manualslib.com/manual/2391841/Dsc-Pc1555.html)
- [Konnected](https://konnected.io/) — the commercial version of what this repo now builds
