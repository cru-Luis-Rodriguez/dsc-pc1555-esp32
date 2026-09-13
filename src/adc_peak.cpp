/*
 *  ADC peak probe — what voltage does the clock line ACTUALLY reach?
 *
 *  Everything measured so far has been a DC average: the meter at the panel,
 *  the meter at the pin. An average cannot see the peak, and the peak is what
 *  a logic input latches on. pin_probe.cpp showed GPIO 18 never crossing VIH
 *  despite a healthy-looking 0.9 V average, which means the peak is lower than
 *  the average implies. This measures it.
 *
 *  TEMPORARY REWIRE — one wire, then put it back:
 *
 *      Move the CLOCK tap (the 33k/10k junction currently on D18)
 *      to GPIO 34 instead. Nothing else changes. Leave the 10k to
 *      ground, leave the 33k on panel YEL, leave the data divider alone.
 *
 *  GPIO 18 has no ADC. GPIO 34 is ADC1 (usable with WiFi, input-only, fine
 *  here). Panel powered down while you move the wire, per the usual rule.
 *
 *  Build:  pio run -e peak -t upload
 *  Then:   ~/.platformio/penv/bin/python tools/capture_serial.py --reset \
 *              /dev/cu.usbserial-0001 115200 15 peak.log
 *
 *  Reading the result — divider ratio is 0.2282 (32.9k / 9.73k measured), so
 *  multiply the pin voltage by 4.38 to recover the level at panel YEL:
 *
 *    max ~3100 mV (saturated)   YEL is reaching ~13.6 V. Levels are fine and
 *                               the fault is protocol, not electrical.
 *                               ESP32 ADC saturates near 3.1 V with 11dB
 *                               attenuation, so this reads as "at least".
 *
 *    max ~2500 mV               YEL peaks near 11 V. Marginal — right on VIH.
 *
 *    max ~1800 mV               YEL peaks near 8 V. Well under VIH. The panel
 *                               is not driving the clock to a proper logic
 *                               level. That is a panel fault and it explains
 *                               the keypads receiving nothing usable either.
 *
 *    max ~= min                 Line is not switching at all. Steady DC.
 *
 *  The min should sit near 0 mV in all healthy cases — the line does pull low.
 *
 *  AFTER RUNNING: move the wire back to D18 and reflash the reader target.
 */

#include <Arduino.h>

#define PROBE_PIN 34   // ADC1 channel 6, input-only. Clock tap goes here.

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("adc peak probe on GPIO 34"));
  Serial.println(F("multiply mV by 4.38 to get the level at panel YEL"));

  analogSetPinAttenuation(PROBE_PIN, ADC_11db);  // full ~0-3.1V range
}

void loop() {
  uint32_t vMin = UINT32_MAX;
  uint32_t vMax = 0;
  uint32_t sum  = 0;
  uint32_t n    = 0;

  uint32_t start = millis();
  while (millis() - start < 1000) {
    uint32_t mv = analogReadMilliVolts(PROBE_PIN);
    if (mv < vMin) vMin = mv;
    if (mv > vMax) vMax = mv;
    sum += mv;
    n++;
  }

  Serial.printf(
    "min %4u mV   max %4u mV   avg %4u mV   (%u samples)   "
    "-> YEL peak ~%u mV\n",
    vMin, vMax, (unsigned)(sum / n), n, (unsigned)(vMax * 438 / 100)
  );
}
