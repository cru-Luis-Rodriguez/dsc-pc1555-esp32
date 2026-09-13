#!/usr/bin/env python3
"""
Capture N seconds of serial output to a file, non-interactively.

Why this exists:
  - `pio device monitor` requires a tty. Redirect its output and it dies with
    termios.error: (102, 'Operation not supported on socket'), so it cannot be
    used to capture output from a script or an agent session.
  - macOS has no `timeout` (that's GNU coreutils).

Usage:
    python3 tools/capture_serial.py [--reset] [port] [baud] [seconds] [outfile]

Defaults: /dev/cu.usbserial-0001 115200 30 serial-capture.log

    --reset   Pulse the board's reset line before capturing, so the boot banner
              lands inside the window. Use this when you need to prove the board
              is actually running.

Needs pyserial. PlatformIO bundles it, so the reliable invocation is:

    ~/.platformio/penv/bin/python tools/capture_serial.py --reset

Use that path, not a Homebrew Cellar path — the Cellar path contains the
PlatformIO version number and breaks on every upgrade.

DTR/RTS
-------
On ESP32 dev boards the USB-serial chip's DTR and RTS lines drive EN (reset)
and GPIO0 (boot select) through the auto-reset transistors. pyserial asserts
both by default when opening a port, which can leave the board **held in
reset** — the port opens cleanly and you get zero bytes indefinitely.

So this script explicitly deasserts both after opening. With --reset it then
performs the standard sequence (RTS low = EN low, pause, release) to reboot the
board into run mode, never into download mode.
"""

import sys
import time

try:
    import serial  # pyserial
except ImportError:
    sys.exit(
        "pyserial not found.\n"
        "Run this with PlatformIO's bundled python instead:\n"
        "    ~/.platformio/penv/bin/python tools/capture_serial.py"
    )

DEFAULTS = ("/dev/cu.usbserial-0001", 115200, 30, "serial-capture.log")


def pulse_reset(ser: "serial.Serial") -> None:
    """Reboot an ESP32 into run mode via the auto-reset circuit.

    RTS drives EN (reset), DTR drives GPIO0 (boot select). Holding GPIO0 high
    while releasing EN gives a normal boot rather than download mode.
    """
    ser.dtr = False   # GPIO0 high -> normal boot, not download mode
    ser.rts = True    # EN low     -> hold in reset
    time.sleep(0.15)
    ser.rts = False   # EN high    -> release, board boots
    time.sleep(0.05)


def main() -> int:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = {a for a in sys.argv[1:] if a.startswith("--")}
    do_reset = "--reset" in flags

    port = args[0] if len(args) > 0 else DEFAULTS[0]
    baud = int(args[1]) if len(args) > 1 else DEFAULTS[1]
    seconds = float(args[2]) if len(args) > 2 else DEFAULTS[2]
    outfile = args[3] if len(args) > 3 else DEFAULTS[3]

    try:
        ser = serial.Serial(port, baud, timeout=0.2)
    except serial.SerialException as exc:
        sys.exit(f"could not open {port}: {exc}")

    # Never leave the board held in reset. See the DTR/RTS note above.
    try:
        ser.dtr = False
        ser.rts = False
    except OSError as exc:
        print(f"# warning: could not set DTR/RTS: {exc}", file=sys.stderr)

    if do_reset:
        print("# pulsing reset", file=sys.stderr, flush=True)
        try:
            pulse_reset(ser)
        except OSError as exc:
            print(f"# warning: reset pulse failed: {exc}", file=sys.stderr)
        ser.reset_input_buffer()

    print(
        f"# capturing {seconds:g}s from {port} @ {baud} -> {outfile}",
        file=sys.stderr,
        flush=True,
    )

    deadline = time.monotonic() + seconds
    total = 0

    with ser, open(outfile, "wb") as fh:
        while time.monotonic() < deadline:
            chunk = ser.read(4096)
            if not chunk:
                continue
            total += len(chunk)
            fh.write(chunk)
            fh.flush()
            sys.stdout.write(chunk.decode("utf-8", errors="replace"))
            sys.stdout.flush()

    print(f"\n# {total} bytes in {seconds:g}s", file=sys.stderr, flush=True)

    if total == 0:
        print(
            "# zero bytes. If --reset was used and even the boot banner is "
            "missing, the board is not running — suspect a short on EN or a "
            "stray strand bridging pins, not the Keybus.",
            file=sys.stderr,
            flush=True,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
