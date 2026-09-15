/*
 *  Direct zone-loop reading — shared module for the zone-serial and zone-web
 *  targets. Replaces dscKeybusInterface after the PC1555 was retired
 *  (see docs/panel-bringup.md → "Final test session").
 *
 *  Per-zone circuit (docs/diy-zone-reader.md, step 2), assuming 5.6k EOL:
 *
 *      3.3V ──R1 5.6k──┬──R3 1k── ADC pin ──C1 0.1µF── GND
 *                      ├──R2 15k── GND
 *                      └── zone loop (contacts + EOL) ── GND
 *
 *  Expected node voltages:  short ~0 V · normal ~1.39 V · open ~2.40 V
 *
 *  The thresholds below assume the DSC-standard 5.6 kΩ EOL resistor, which is
 *  UNVERIFIED until Stage 1b is done. If the loops measure differently, adjust
 *  the four *_MV constants to sit between the three bench-measured states —
 *  empirical beats calculated.
 *
 *  All pins are ADC1 (GPIO 32-39): ADC2 is unusable while WiFi is active.
 *  GPIO 36/39 glitch when WiFi power-save is on — every WiFi target must call
 *  WiFi.setSleep(false).
 */

#pragma once
#include <Arduino.h>

#define ZONE_COUNT 5

// Classification thresholds, millivolts at the ADC pin.
#define ZONE_SHORT_MAX_MV   700   // below: loop shorted (tamper / crushed wire)
#define ZONE_NORMAL_MIN_MV  900   // 900-1900: contacts closed through the EOL
#define ZONE_NORMAL_MAX_MV 1900
#define ZONE_OPEN_MIN_MV   2000   // above: contact open, or a cut wire

// 700-899 and 1901-1999 are hysteresis bands: the previous state is retained.

#define ZONE_POLL_MS        100   // per-pass sampling interval
#define ZONE_DEBOUNCE_READS   2   // consecutive agreeing reads to change state

enum ZoneState : uint8_t {
  ZONE_UNKNOWN = 0,   // not yet sampled, or stuck in a hysteresis band since boot
  ZONE_NORMAL,        // closed through the EOL resistor
  ZONE_OPEN,          // triggered, or broken wire
  ZONE_SHORT          // tamper
};

struct ZoneConfig {
  uint8_t     pin;    // ADC1 GPIO
  const char* name;   // from the keypad label card
};

extern const ZoneConfig kZones[ZONE_COUNT];

const char* zoneStateName(ZoneState s);

// Configure ADC attenuation and take the first sample of every zone.
void zonesBegin();

// Sample all zones if ZONE_POLL_MS has elapsed (cheap no-op otherwise).
// For every debounced state change, calls onChange(index, oldState, newState)
// — pass nullptr if not needed. Returns true if anything changed this call.
bool zonesPoll(void (*onChange)(uint8_t idx, ZoneState oldState, ZoneState newState));

ZoneState zoneState(uint8_t idx);   // last debounced state
uint16_t  zoneMv(uint8_t idx);      // last median reading, millivolts
