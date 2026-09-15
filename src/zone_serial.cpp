/*
 *  Zone status over serial — direct loop reading, no panel.
 *
 *  Successor to status_serial.cpp (kept as the keybus-serial reference env)
 *  after the PC1555 was retired. Prints a boot snapshot of every zone, then
 *  one line per debounced state change, with the measured voltage — so the
 *  first bench session doubles as threshold verification.
 *
 *  Build:  pio run -e serial -t upload -t monitor
 *  Capture (agent/scripted):
 *          ~/.platformio/penv/bin/python tools/capture_serial.py --reset
 *
 *  Wiring: docs/diy-zone-reader.md → "This installation, concretely".
 *  Send any character over serial to reprint the full snapshot.
 */

#include <Arduino.h>
#include "zones.h"

static void printZone(uint8_t i) {
  Serial.printf("zone %u  %-16s GPIO%-2u  %4u mV  %s\n",
                i + 1, kZones[i].name, kZones[i].pin,
                zoneMv(i), zoneStateName(zoneState(i)));
}

static void printSnapshot() {
  Serial.println(F("---- zone snapshot ----"));
  for (uint8_t i = 0; i < ZONE_COUNT; i++) printZone(i);
  Serial.println(F("-----------------------"));
}

static void onZoneChange(uint8_t i, ZoneState oldState, ZoneState newState) {
  Serial.printf("[%10lu ms] zone %u  %-16s %s -> %s  (%u mV)\n",
                millis(), i + 1, kZones[i].name,
                zoneStateName(oldState), zoneStateName(newState), zoneMv(i));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("zone reader: direct loop sensing on ADC1 (no panel)"));
  Serial.printf("thresholds mV: short<%u  normal %u-%u  open>=%u\n",
                ZONE_SHORT_MAX_MV, ZONE_NORMAL_MIN_MV,
                ZONE_NORMAL_MAX_MV, ZONE_OPEN_MIN_MV);
  zonesBegin();
  printSnapshot();
}

void loop() {
  zonesPoll(onZoneChange);

  if (Serial.available() > 0) {
    while (Serial.available() > 0) Serial.read();
    printSnapshot();
  }
}
